// SPDX-License-Identifier: GPL-2.0
/*
 * FPGA Manager Driver for Intel Stratix10 SoC
 *
 *  Copyright (C) 2018 Intel Corporation
 */
#include <linux/completion.h>
#include <linux/fpga/fpga-mgr.h>
#include <linux/firmware/intel/stratix10-svc-client.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/dma-mapping.h>

/*
 * FPGA programming requires a higher level of privilege (EL3), per the SoC
 * design.
 */
#define NUM_SVC_BUFS	4
#define SVC_BUF_SIZE	SZ_512K

/* Indicates buffer is in use if set */
#define SVC_BUF_LOCK	0

#define S10_BUFFER_TIMEOUT (msecs_to_jiffies(SVC_RECONFIG_BUFFER_TIMEOUT_MS))
#define S10_RECONFIG_TIMEOUT (msecs_to_jiffies(SVC_RECONFIG_REQUEST_TIMEOUT_MS))

#define INVALID_FIRMWARE_VERSION	0xFFFF
typedef void (*s10_callback)(struct stratix10_svc_client *client,
			     struct stratix10_svc_cb_data *data);

/*
 * struct s10_svc_buf
 * buf:  virtual address of buf provided by service layer
 * lock: locked if buffer is in use
 */
struct s10_svc_buf {
	char *buf;
	dma_addr_t dma_addr;
	unsigned long lock;
};

struct s10_priv {
	struct stratix10_svc_chan *chan;
	struct stratix10_svc_client client;
	struct completion status_return_completion;
	struct s10_svc_buf svc_bufs[NUM_SVC_BUFS];
	unsigned long status;
	unsigned int fw_version;
	bool is_smmu_enabled;
};

static int s10_svc_send_msg(struct s10_priv *priv,
			    enum stratix10_svc_command_code command,
			    void *payload, u32 payload_length,
			    s10_callback callback)
{
	struct stratix10_svc_chan *chan = priv->chan;
	struct device *dev = priv->client.dev;
	struct stratix10_svc_client_msg msg;
	int ret;

	dev_dbg(dev, "%s cmd=%d payload=%p length=%d\n",
		__func__, command, payload, payload_length);

	msg.command = command;
	msg.payload = payload;
	msg.payload_length = payload_length;
	priv->client.receive_cb = callback;

	ret = stratix10_svc_send(chan, &msg);
	dev_dbg(dev, "stratix10_svc_send returned status %d\n", ret);

	return ret;
}

/*
 * Free buffers allocated from the service layer's pool that are not in use.
 * Return true when all buffers are freed.
 */
static bool s10_free_buffers(struct fpga_manager *mgr)
{
	struct s10_priv *priv = mgr->priv;
	uint num_free = 0;
	uint i;

	for (i = 0; i < NUM_SVC_BUFS; i++) {
		if (!priv->svc_bufs[i].buf) {
			num_free++;
			continue;
		}

		if (!test_and_set_bit_lock(SVC_BUF_LOCK,
					   &priv->svc_bufs[i].lock)) {
			stratix10_svc_free_memory(priv->chan,
						  priv->svc_bufs[i].buf);
			priv->svc_bufs[i].buf = NULL;
			num_free++;
		}
	}

	return num_free == NUM_SVC_BUFS;
}

/*
 * Returns count of how many buffers are not in locked state.
 */
static uint s10_get_unlocked_buffer_count(struct fpga_manager *mgr)
{
	struct s10_priv *priv = mgr->priv;
	uint num_free = 0;
	uint i;

	for (i = 0; i < NUM_SVC_BUFS; i++)
		if (!priv->svc_bufs[i].lock)
			num_free++;

	return num_free;
}

/*
 * s10_unlock_bufs
 * Given the returned buffer address, match that address to our buffer struct
 * and unlock that buffer.  This marks it as available to be refilled and sent
 * (or freed).
 * priv: private data
 * kaddr: kernel address of buffer that was returned from service layer
 */
static void s10_unlock_bufs(struct s10_priv *priv, void *kaddr)
{
	uint i;

	if (!kaddr)
		return;

	for (i = 0; i < NUM_SVC_BUFS; i++)
		if (priv->svc_bufs[i].buf == kaddr) {
			if (priv->is_smmu_enabled == true)
				dma_unmap_single(priv->client.dev, priv->svc_bufs[i].dma_addr, SVC_BUF_SIZE, DMA_TO_DEVICE);
			clear_bit_unlock(SVC_BUF_LOCK,
					 &priv->svc_bufs[i].lock);
			return;
		}

	WARN(1, "Unknown buffer returned from service layer %p\n", kaddr);
}

/*
 * s10_fw_version_callback - callback for the version of running firmware
 * @client: service layer client struct
 * @data: message from service layer
 */
static void s10_fw_version_callback(struct stratix10_svc_client *client,
				    struct stratix10_svc_cb_data *data)
{
	struct s10_priv *priv = client->priv;
	unsigned int *version = (unsigned int *)data->kaddr1;

	if (data->status == BIT(SVC_STATUS_OK))
		priv->fw_version = *version;
	else if (data->status == BIT(SVC_STATUS_NO_SUPPORT))
		dev_warn(client->dev,
			 "FW doesn't support bitstream authentication\n");
	else
		dev_err(client->dev, "Failed to get FW version %lu\n",
			BIT(data->status));

	complete(&priv->status_return_completion);
}

/*
 * s10_receive_callback - callback for service layer to use to provide client
 * (this driver) messages received through the mailbox.
 * client: service layer client struct
 * data: message from service layer
 */
static void s10_receive_callback(struct stratix10_svc_client *client,
				 struct stratix10_svc_cb_data *data)
{
	struct s10_priv *priv = client->priv;
	u32 status;
	int i;

	pr_debug("%s data %x\n", __func__, data->status);
	WARN_ONCE(!data, "%s: stratix10_svc_rc_data = NULL", __func__);

	status = data->status;

	/*
	 * Here we set status bits as we receive them.  Elsewhere, we always use
	 * test_and_clear_bit() to check status in priv->status
	 */
	for (i = 0; i <= SVC_STATUS_ERROR; i++)
		if (status & (1 << i))
			set_bit(i, &priv->status);

	if (status & BIT(SVC_STATUS_BUFFER_DONE)) {
		s10_unlock_bufs(priv, data->kaddr1);
		s10_unlock_bufs(priv, data->kaddr2);
		s10_unlock_bufs(priv, data->kaddr3);
		s10_unlock_bufs(priv, data->kaddr4);
	}

	complete(&priv->status_return_completion);
}

/*
 * s10_ops_write_init - prepare for FPGA reconfiguration by requesting
 * partial reconfig and allocating buffers from the service layer.
 */
static int s10_ops_write_init(struct fpga_manager *mgr,
			      struct fpga_image_info *info,
			      const char *buf, size_t count)
{
	struct s10_priv *priv = mgr->priv;
	struct device *dev = priv->client.dev;
	struct stratix10_svc_command_config_type ctype;
	uint i;
	int ret;

	ctype.flags = 0;
	if (info->flags & FPGA_MGR_PARTIAL_RECONFIG) {
		dev_dbg(dev, "Requesting partial reconfiguration.\n");
		ctype.flags |= BIT(COMMAND_RECONFIG_FLAG_PARTIAL);
	} else if (info->flags & FPGA_MGR_BITSTREAM_AUTHENTICATE) {
		if (priv->fw_version == INVALID_FIRMWARE_VERSION) {
			dev_err(dev, "FW doesn't support\n");
			return -EINVAL;
		}

		dev_dbg(dev, "Requesting bitstream authentication.\n");
		ctype.flags |= BIT(COMMAND_AUTHENTICATE_BITSTREAM);
	} else {
		dev_dbg(dev, "Requesting full reconfiguration.\n");
	}

	reinit_completion(&priv->status_return_completion);
	ret = s10_svc_send_msg(priv, COMMAND_RECONFIG,
			       &ctype, sizeof(ctype),
			       s10_receive_callback);
	if (ret < 0)
		goto init_done;

	ret = wait_for_completion_timeout(
		&priv->status_return_completion, S10_RECONFIG_TIMEOUT);
	if (!ret) {
		dev_err(dev, "timeout waiting for RECONFIG_REQUEST\n");
		ret = -ETIMEDOUT;
		goto init_done;
	}

	ret = 0;
	if (!test_and_clear_bit(SVC_STATUS_OK, &priv->status)) {
		ret = -ETIMEDOUT;
		goto init_done;
	}

	/* Init buffer lock */
	for (i = 0; i < NUM_SVC_BUFS; i++) {
		priv->svc_bufs[i].lock = 0;
	}

init_done:
	stratix10_svc_done(priv->chan);
	return ret;
}

/*
 * s10_send_buf - send a buffer to the service layer queue
 * mgr: fpga manager struct
 * buf: fpga image buffer
 * count: size of buf in bytes
 * Returns # of bytes transferred or -ENOBUFS if the all the buffers are in use
 * or if the service queue is full. Never returns 0.
 */
static int s10_send_buf(struct fpga_manager *mgr, const char *buf, size_t count)
{
	struct s10_priv *priv = mgr->priv;
	struct device *dev = priv->client.dev;
	void *svc_buf;
	size_t xfer_sz;
	int ret;
	uint i;

	/* get/lock a buffer that that's not being used */
	for (i = 0; i < NUM_SVC_BUFS; i++)
		if (!test_and_set_bit_lock(SVC_BUF_LOCK,
					   &priv->svc_bufs[i].lock))
			break;

	if (i == NUM_SVC_BUFS)
		return -ENOBUFS;

	xfer_sz = count < SVC_BUF_SIZE ? count : SVC_BUF_SIZE;

	svc_buf = priv->svc_bufs[i].buf;
	memcpy(svc_buf, buf, xfer_sz);
	if (priv->is_smmu_enabled == true)
		priv->svc_bufs[i].dma_addr = dma_map_single(dev, svc_buf, SVC_BUF_SIZE, DMA_TO_DEVICE);

	ret = s10_svc_send_msg(priv, COMMAND_RECONFIG_DATA_SUBMIT,
			       svc_buf, xfer_sz, s10_receive_callback);
	if (ret < 0) {
		dev_err(dev,
			"Error while sending data to service layer (%d)", ret);
		clear_bit_unlock(SVC_BUF_LOCK, &priv->svc_bufs[i].lock);
		return ret;
	}

	return xfer_sz;
}

/*
 * Send an FPGA image to privileged layers to write to the FPGA.  When done
 * sending, free all service layer buffers we allocated in write_init.
 */
static int s10_ops_write(struct fpga_manager *mgr, const char *buf,
			 size_t count)
{
	struct s10_priv *priv = mgr->priv;
	struct device *dev = priv->client.dev;
	long wait_status;
	int sent = 0;
	int ret = 0;

	/*
	 * Loop waiting for buffers to be returned.  When a buffer is returned,
	 * reuse it to send more data or free if if all data has been sent.
	 */
	while (true) {
		reinit_completion(&priv->status_return_completion);

		if (count > 0) {
			sent = s10_send_buf(mgr, buf, count);
			if (sent < 0)
				continue;

			count -= sent;
			buf += sent;
		} else {
			if (s10_get_unlocked_buffer_count(mgr) == NUM_SVC_BUFS)
				return 0;

			ret = s10_svc_send_msg(
				priv, COMMAND_RECONFIG_DATA_CLAIM,
				NULL, 0, s10_receive_callback);
			if (ret < 0)
				break;
		}

		wait_status = wait_for_completion_timeout(
				&priv->status_return_completion,
				S10_BUFFER_TIMEOUT);

		if (test_and_clear_bit(SVC_STATUS_BUFFER_DONE, &priv->status) ||
		    test_and_clear_bit(SVC_STATUS_BUFFER_SUBMITTED,
				       &priv->status)) {
			ret = 0;
			continue;
		}

		if (test_and_clear_bit(SVC_STATUS_ERROR, &priv->status)) {
			dev_err(dev, "ERROR - giving up - SVC_STATUS_ERROR\n");
			ret = -EFAULT;
			break;
		}

		if (!wait_status) {
			dev_err(dev, "timeout waiting for svc layer buffers\n");
			ret = -ETIMEDOUT;
			break;
		}
	}

	return ret;
}

static int s10_ops_write_complete(struct fpga_manager *mgr,
				  struct fpga_image_info *info)
{
	struct s10_priv *priv = mgr->priv;
	struct device *dev = priv->client.dev;
	unsigned long timeout;
	int ret;

	/* The time taken to process this is close to 600ms
	 * This MUST be increased over 1 second
	 */
	timeout = S10_RECONFIG_TIMEOUT;

	do {
		reinit_completion(&priv->status_return_completion);

		ret = s10_svc_send_msg(priv, COMMAND_RECONFIG_STATUS,
				       NULL, 0, s10_receive_callback);
		if (ret < 0)
			break;

		ret = wait_for_completion_timeout(
			&priv->status_return_completion, timeout);
		if (!ret) {
			dev_err(dev,
				"timeout waiting for RECONFIG_COMPLETED\n");
			ret = -ETIMEDOUT;
			break;
		}
		/* Not error or timeout, so ret is # of jiffies until timeout */
		timeout = ret;
		ret = 0;

		if (test_and_clear_bit(SVC_STATUS_COMPLETED, &priv->status))
			break;

		if (test_and_clear_bit(SVC_STATUS_ERROR, &priv->status)) {
			dev_err(dev, "ERROR - giving up - SVC_STATUS_ERROR\n");
			ret = -EFAULT;
			break;
		}
	} while (1);

	stratix10_svc_done(priv->chan);

	return ret;
}

static const struct fpga_manager_ops s10_ops = {
	.write_init = s10_ops_write_init,
	.write = s10_ops_write,
	.write_complete = s10_ops_write_complete,
};

static int s10_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct s10_priv *priv;
	struct fpga_manager *mgr;
	int ret, i;
	struct device_node *node = pdev->dev.of_node;
	char *kbuf;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->fw_version = INVALID_FIRMWARE_VERSION;
	priv->client.dev = dev;
	priv->client.receive_cb = NULL;
	priv->client.priv = priv;
	priv->is_smmu_enabled = false;

	if (of_device_is_compatible(node, "intel,agilex5-soc-fpga-mgr"))
		priv->is_smmu_enabled = device_property_read_bool(dev,
									"altr,smmu_enable_quirk");


	priv->chan = stratix10_svc_request_channel_byname(&priv->client,
							  SVC_CLIENT_FPGA);
	if (IS_ERR(priv->chan)) {
		dev_err(dev, "couldn't get service channel (%s)\n",
			SVC_CLIENT_FPGA);
		return PTR_ERR(priv->chan);
	}

	init_completion(&priv->status_return_completion);

	mgr = fpga_mgr_register(dev, "Stratix10 SOC FPGA Manager",
				&s10_ops, priv);
	if (IS_ERR(mgr)) {
		dev_err(dev, "unable to register FPGA manager\n");
		ret = PTR_ERR(mgr);
		goto probe_err;
	}

	/* get the running firmware version */
	ret = s10_svc_send_msg(priv, COMMAND_FIRMWARE_VERSION,
			       NULL, 0, s10_fw_version_callback);
	if (ret) {
		dev_err(dev, "couldn't get firmware version\n");
		stratix10_svc_done(priv->chan);
		goto probe_err;
	}

	ret = wait_for_completion_timeout(
		&priv->status_return_completion, S10_RECONFIG_TIMEOUT);
	if (!ret) {
		dev_err(dev, "timeout waiting for firmware version\n");
		stratix10_svc_done(priv->chan);
		ret = -ETIMEDOUT;
		goto probe_err;
	}

	ret = 0;

	/* Allocate buffers from the service layer's pool. */
	for (i = 0; i < NUM_SVC_BUFS; i++) {
		kbuf = stratix10_svc_allocate_memory(priv->chan, SVC_BUF_SIZE);
		if (IS_ERR(kbuf)) {
			s10_free_buffers(mgr);
			ret = PTR_ERR(kbuf);
			goto probe_err;
		}

		priv->svc_bufs[i].buf = kbuf;
		priv->svc_bufs[i].lock = 0;
	}

	stratix10_svc_done(priv->chan);
	platform_set_drvdata(pdev, mgr);
	return 0;

probe_err:
	stratix10_svc_free_channel(priv->chan);
	return ret;
}

static int s10_remove(struct platform_device *pdev)
{
	struct fpga_manager *mgr = platform_get_drvdata(pdev);
	struct s10_priv *priv = mgr->priv;
	int i;

	for (i = 0; i < NUM_SVC_BUFS; i++) {
		if (priv->svc_bufs[i].buf)
			stratix10_svc_free_memory(priv->chan,
					  priv->svc_bufs[i].buf);
	}

	fpga_mgr_unregister(mgr);
	stratix10_svc_free_channel(priv->chan);

	return 0;
}

static const struct of_device_id s10_of_match[] = {
	{.compatible = "intel,stratix10-soc-fpga-mgr"},
	{.compatible = "intel,agilex-soc-fpga-mgr"},
	{.compatible = "intel,agilex5-soc-fpga-mgr"},
	{},
};

MODULE_DEVICE_TABLE(of, s10_of_match);

static struct platform_driver s10_driver = {
	.probe = s10_probe,
	.remove = s10_remove,
	.driver = {
		.name	= "Stratix10 SoC FPGA manager",
		.of_match_table = of_match_ptr(s10_of_match),
	},
};

static int __init s10_init(void)
{
	struct device_node *fw_np;
	struct device_node *np;
	int ret;

	fw_np = of_find_node_by_name(NULL, "svc");
	if (!fw_np)
		return -ENODEV;

	of_node_get(fw_np);
	np = of_find_matching_node(fw_np, s10_of_match);
	if (!np) {
		of_node_put(fw_np);
		return -ENODEV;
	}

	of_node_put(np);
	ret = of_platform_populate(fw_np, s10_of_match, NULL, NULL);
	of_node_put(fw_np);
	if (ret)
		return ret;

	return platform_driver_register(&s10_driver);
}

static void __exit s10_exit(void)
{
	return platform_driver_unregister(&s10_driver);
}

module_init(s10_init);
module_exit(s10_exit);

MODULE_AUTHOR("Alan Tull <atull@kernel.org>");
MODULE_DESCRIPTION("Intel Stratix 10 SOC FPGA Manager");
MODULE_LICENSE("GPL v2");
