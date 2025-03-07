/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2017-2018, Intel Corporation
 */

#ifndef __STRATIX10_SMC_H
#define __STRATIX10_SMC_H

#include <linux/arm-smccc.h>
#include <linux/bitops.h>

/**
 * This file defines the Secure Monitor Call (SMC) message protocol used for
 * service layer driver in normal world (EL1) to communicate with secure
 * monitor software in Secure Monitor Exception Level 3 (EL3).
 *
 * This file is shared with secure firmware (FW) which is out of kernel tree.
 *
 * An ARM SMC instruction takes a function identifier and up to 6 64-bit
 * register values as arguments, and can return up to 4 64-bit register
 * value. The operation of the secure monitor is determined by the parameter
 * values passed in through registers.
 *
 * EL1 and EL3 communicates pointer as physical address rather than the
 * virtual address.
 *
 * Functions specified by ARM SMC Calling convention:
 *
 * FAST call executes atomic operations, returns when the requested operation
 * has completed.
 * STD call starts a operation which can be preempted by a non-secure
 * interrupt. The call can return before the requested operation has
 * completed.
 *
 * a0..a7 is used as register names in the descriptions below, on arm32
 * that translates to r0..r7 and on arm64 to w0..w7.
 */

/**
 * @func_num: function ID
 */
#define INTEL_SIP_SMC_STD_CALL_VAL(func_num) \
	ARM_SMCCC_CALL_VAL(ARM_SMCCC_STD_CALL, ARM_SMCCC_SMC_64, \
	ARM_SMCCC_OWNER_SIP, (func_num))

#define INTEL_SIP_SMC_FAST_CALL_VAL(func_num) \
	ARM_SMCCC_CALL_VAL(ARM_SMCCC_FAST_CALL, ARM_SMCCC_SMC_64, \
	ARM_SMCCC_OWNER_SIP, (func_num))

/**
 * Return values in INTEL_SIP_SMC_* call
 *
 * INTEL_SIP_SMC_RETURN_UNKNOWN_FUNCTION:
 * Secure monitor software doesn't recognize the request.
 *
 * INTEL_SIP_SMC_STATUS_OK:
 * Secure monitor software accepts the service client's request.
 *
 * INTEL_SIP_SMC_STATUS_BUSY:
 * Secure monitor software is still processing service client's request.
 *
 * INTEL_SIP_SMC_STATUS_REJECTED:
 * Secure monitor software reject the service client's request.
 *
 * INTEL_SIP_SMC_STATUS_NO_RESPONSE:
 * Secure monitor software doesn't recieve any response for the
 * service client's request yet.
 *
 * INTEL_SIP_SMC_STATUS_ERROR:
 * There is error during the process of service request.
 *
 * INTEL_SIP_SMC_RSU_ERROR:
 * There is error during the process of remote status update request.
 *
 * INTEL_SIP_SMC_STATUS_NOT_SUPPORTED:
 * Secure monitor software doesn't support the request
 */
#define INTEL_SIP_SMC_RETURN_UNKNOWN_FUNCTION		0xFFFFFFFF
#define INTEL_SIP_SMC_STATUS_OK				0x0
#define INTEL_SIP_SMC_STATUS_BUSY			0x1
#define INTEL_SIP_SMC_STATUS_REJECTED			0x2
#define INTEL_SIP_SMC_STATUS_NO_RESPONSE		0x3
#define INTEL_SIP_SMC_STATUS_ERROR			0x4
#define INTEL_SIP_SMC_RSU_ERROR				0x7
#define INTEL_SIP_SMC_STATUS_NOT_SUPPORTED		0x8

/**
 * Request INTEL_SIP_SMC_FPGA_CONFIG_START
 *
 * Sync call used by service driver at EL1 to request the FPGA in EL3 to
 * be prepare to receive a new configuration.
 *
 * Call register usage:
 * a0: INTEL_SIP_SMC_FPGA_CONFIG_START.
 * a1: flag for full or partial configuration. 0 for full and 1 for partial
 * configuration.
 * a2-7: not used.
 *
 * Return status:
 * a0: INTEL_SIP_SMC_STATUS_OK, or INTEL_SIP_SMC_STATUS_ERROR.
 * a1-3: not used.
 */
#define INTEL_SIP_SMC_FUNCID_FPGA_CONFIG_START 1
#define INTEL_SIP_SMC_FPGA_CONFIG_START \
	INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_FPGA_CONFIG_START)

/**
 * Request INTEL_SIP_SMC_FPGA_CONFIG_WRITE
 *
 * Async call used by service driver at EL1 to provide FPGA configuration data
 * to secure world.
 *
 * Call register usage:
 * a0: INTEL_SIP_SMC_FPGA_CONFIG_WRITE.
 * a1: 64bit physical address of the configuration data memory block
 * a2: Size of configuration data block.
 * a3-7: not used.
 *
 * Return status:
 * a0: INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_BUSY or
 * INTEL_SIP_SMC_STATUS_ERROR.
 * a1: 64bit physical address of 1st completed memory block if any completed
 * block, otherwise zero value.
 * a2: 64bit physical address of 2nd completed memory block if any completed
 * block, otherwise zero value.
 * a3: 64bit physical address of 3rd completed memory block if any completed
 * block, otherwise zero value.
 */
#define INTEL_SIP_SMC_FUNCID_FPGA_CONFIG_WRITE 2
#define INTEL_SIP_SMC_FPGA_CONFIG_WRITE \
	INTEL_SIP_SMC_STD_CALL_VAL(INTEL_SIP_SMC_FUNCID_FPGA_CONFIG_WRITE)

/**
 * Request INTEL_SIP_SMC_FPGA_CONFIG_COMPLETED_WRITE
 *
 * Sync call used by service driver at EL1 to track the completed write
 * transactions. This request is called after INTEL_SIP_SMC_FPGA_CONFIG_WRITE
 * call returns INTEL_SIP_SMC_STATUS_BUSY.
 *
 * Call register usage:
 * a0: INTEL_SIP_SMC_FPGA_CONFIG_COMPLETED_WRITE.
 * a1-7: not used.
 *
 * Return status:
 * a0: INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_FPGA_BUSY or
 * INTEL_SIP_SMC_STATUS_ERROR.
 * a1: 64bit physical address of 1st completed memory block.
 * a2: 64bit physical address of 2nd completed memory block if
 * any completed block, otherwise zero value.
 * a3: 64bit physical address of 3rd completed memory block if
 * any completed block, otherwise zero value.
 */
#define INTEL_SIP_SMC_FUNCID_FPGA_CONFIG_COMPLETED_WRITE 3
#define INTEL_SIP_SMC_FPGA_CONFIG_COMPLETED_WRITE \
INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_FPGA_CONFIG_COMPLETED_WRITE)

/**
 * Request INTEL_SIP_SMC_FPGA_CONFIG_ISDONE
 *
 * Sync call used by service driver at EL1 to inform secure world that all
 * data are sent, to check whether or not the secure world had completed
 * the FPGA configuration process.
 *
 * Call register usage:
 * a0: INTEL_SIP_SMC_FPGA_CONFIG_ISDONE.
 * a1-7: not used.
 *
 * Return status:
 * a0: INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_BUSY or
 * INTEL_SIP_SMC_STATUS_ERROR.
 * a1-3: not used.
 */
#define INTEL_SIP_SMC_FUNCID_FPGA_CONFIG_ISDONE 4
#define INTEL_SIP_SMC_FPGA_CONFIG_ISDONE \
	INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_FPGA_CONFIG_ISDONE)

/**
 * Request INTEL_SIP_SMC_FPGA_CONFIG_GET_MEM
 *
 * Sync call used by service driver at EL1 to query the physical address of
 * memory block reserved by secure monitor software.
 *
 * Call register usage:
 * a0:INTEL_SIP_SMC_FPGA_CONFIG_GET_MEM.
 * a1-7: not used.
 *
 * Return status:
 * a0: INTEL_SIP_SMC_STATUS_OK or INTEL_SIP_SMC_STATUS_ERROR.
 * a1: start of physical address of reserved memory block.
 * a2: size of reserved memory block.
 * a3: not used.
 */
#define INTEL_SIP_SMC_FUNCID_FPGA_CONFIG_GET_MEM 5
#define INTEL_SIP_SMC_FPGA_CONFIG_GET_MEM \
	INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_FPGA_CONFIG_GET_MEM)

/**
 * Request INTEL_SIP_SMC_FPGA_CONFIG_LOOPBACK
 *
 * For SMC loop-back mode only, used for internal integration, debugging
 * or troubleshooting.
 *
 * Call register usage:
 * a0: INTEL_SIP_SMC_FPGA_CONFIG_LOOPBACK.
 * a1-7: not used.
 *
 * Return status:
 * a0: INTEL_SIP_SMC_STATUS_OK or INTEL_SIP_SMC_STATUS_ERROR.
 * a1-3: not used.
 */
#define INTEL_SIP_SMC_FUNCID_FPGA_CONFIG_LOOPBACK 6
#define INTEL_SIP_SMC_FPGA_CONFIG_LOOPBACK \
	INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_FPGA_CONFIG_LOOPBACK)

/**
 * Request INTEL_SIP_SMC_REG_READ
 *
 * Read a protected register at EL3
 *
 * Call register usage:
 * a0: INTEL_SIP_SMC_REG_READ.
 * a1: register address.
 * a2-7: not used.
 *
 * Return status:
 * a0: INTEL_SIP_SMC_STATUS_OK or INTEL_SIP_SMC_REG_ERROR.
 * a1: value in the register
 * a2-3: not used.
 */
#define INTEL_SIP_SMC_FUNCID_REG_READ 7
#define INTEL_SIP_SMC_REG_READ \
	INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_REG_READ)

/**
 * Request INTEL_SIP_SMC_REG_WRITE
 *
 * Write a protected register at EL3
 *
 * Call register usage:
 * a0: INTEL_SIP_SMC_REG_WRITE.
 * a1: register address
 * a2: value to program into register.
 * a3-7: not used.
 *
 * Return status:
 * a0: INTEL_SIP_SMC_STATUS_OK or INTEL_SIP_SMC_REG_ERROR.
 * a1-3: not used.
 */
#define INTEL_SIP_SMC_FUNCID_REG_WRITE 8
#define INTEL_SIP_SMC_REG_WRITE \
	INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_REG_WRITE)

/**
 * Request INTEL_SIP_SMC_FUNCID_REG_UPDATE
 *
 * Update one or more bits in a protected register at EL3 using a
 * read-modify-write operation.
 *
 * Call register usage:
 * a0: INTEL_SIP_SMC_REG_UPDATE.
 * a1: register address
 * a2: write Mask.
 * a3: value to write.
 * a4-7: not used.
 *
 * Return status:
 * a0: INTEL_SIP_SMC_STATUS_OK or INTEL_SIP_SMC_REG_ERROR.
 * a1-3: Not used.
 */
#define INTEL_SIP_SMC_FUNCID_REG_UPDATE 9
#define INTEL_SIP_SMC_REG_UPDATE \
	INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_REG_UPDATE)

/**
 * Request INTEL_SIP_SMC_RSU_STATUS
 *
 * Request remote status update boot log, call is synchronous.
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_RSU_STATUS
 * a1-7 not used
 *
 * Return status
 * a0: Current Image
 * a1: Last Failing Image
 * a2: Version | State
 * a3: Error details | Error location
 *
 * Or
 *
 * a0: INTEL_SIP_SMC_RSU_ERROR
 */
#define INTEL_SIP_SMC_FUNCID_RSU_STATUS 11
#define INTEL_SIP_SMC_RSU_STATUS \
	INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_RSU_STATUS)

/**
 * Request INTEL_SIP_SMC_RSU_UPDATE
 *
 * Request to set the offset of the bitstream to boot after reboot, call
 * is synchronous.
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_RSU_UPDATE
 * a1 64bit physical address of the configuration data memory in flash
 * a2-7 not used
 *
 * Return status
 * a0 INTEL_SIP_SMC_STATUS_OK
 */
#define INTEL_SIP_SMC_FUNCID_RSU_UPDATE 12
#define INTEL_SIP_SMC_RSU_UPDATE \
	INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_RSU_UPDATE)

/**
 * Request INTEL_SIP_SMC_ECC_DBE
 *
 * Sync call used by service driver at EL1 to alert EL3 that a Double
 * Bit ECC error has occurred.
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_ECC_DBE
 * a1 SysManager Double Bit Error value
 * a2-7 not used
 *
 * Return status
 * a0 INTEL_SIP_SMC_STATUS_OK
 */
#define INTEL_SIP_SMC_FUNCID_ECC_DBE 13
#define INTEL_SIP_SMC_ECC_DBE \
	INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_ECC_DBE)

/**
 * Request INTEL_SIP_SMC_RSU_NOTIFY
 *
 * Sync call used by service driver at EL1 to report hard processor
 * system execution stage to firmware
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_RSU_NOTIFY
 * a1 32bit value representing hard processor system execution stage
 * a2-7 not used
 *
 * Return status
 * a0 INTEL_SIP_SMC_STATUS_OK
 */
#define INTEL_SIP_SMC_FUNCID_RSU_NOTIFY 14
#define INTEL_SIP_SMC_RSU_NOTIFY \
	INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_RSU_NOTIFY)

/**
 * Request INTEL_SIP_SMC_RSU_RETRY_COUNTER
 *
 * Sync call used by service driver at EL1 to query RSU retry counter
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_RSU_RETRY_COUNTER
 * a1-7 not used
 *
 * Return status
 * a0 INTEL_SIP_SMC_STATUS_OK
 * a1 the retry counter
 *
 * Or
 *
 * a0 INTEL_SIP_SMC_RSU_ERROR
 */
#define INTEL_SIP_SMC_FUNCID_RSU_RETRY_COUNTER 15
#define INTEL_SIP_SMC_RSU_RETRY_COUNTER \
	INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_RSU_RETRY_COUNTER)

/**
 * Request INTEL_SIP_SMC_RSU_DCMF_VERSION
 *
 * Sync call used by service driver at EL1 to query DCMF (Decision
 * Configuration Management Firmware) version from FW
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_RSU_DCMF_VERSION
 * a1-7 not used
 *
 * Return status
 * a0 INTEL_SIP_SMC_STATUS_OK
 * a1 dcmf1 | dcmf0
 * a2 dcmf3 | dcmf2
 *
 * Or
 *
 * a0 INTEL_SIP_SMC_RSU_ERROR
 */
#define INTEL_SIP_SMC_FUNCID_RSU_DCMF_VERSION 16
#define INTEL_SIP_SMC_RSU_DCMF_VERSION \
	INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_RSU_DCMF_VERSION)

/**
 * Request INTEL_SIP_SMC_RSU_MAX_RETRY
 *
 * Sync call used by service driver at EL1 to query max retry value from FW
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_RSU_MAX_RETRY
 * a1-7 not used
 *
 * Return status
 * a0 INTEL_SIP_SMC_STATUS_OK
 * a1 max retry value
 *
 * Or
 * a0 INTEL_SIP_SMC_RSU_ERROR
 */
#define INTEL_SIP_SMC_FUNCID_RSU_MAX_RETRY 18
#define INTEL_SIP_SMC_RSU_MAX_RETRY \
       INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_RSU_MAX_RETRY)

/**
 * Request INTEL_SIP_SMC_RSU_DCMF_STATUS
 *
 * Sync call used by service driver at EL1 to query DCMF status from FW
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_RSU_DCMF_STATUS
 * a1-7 not used
 *
 * Return status
 * a0 INTEL_SIP_SMC_STATUS_OK
 * a1 dcmf3 | dcmf2 | dcmf1 | dcmf0
 *
 * Or
 *
 * a0 INTEL_SIP_SMC_RSU_ERROR
 */
#define INTEL_SIP_SMC_FUNCID_RSU_DCMF_STATUS 20
#define INTEL_SIP_SMC_RSU_DCMF_STATUS \
	INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_RSU_DCMF_STATUS)

/**
 * Request INTEL_SIP_SMC_RSU_GET_DEVICE_INFO
 *
 * Sync call used by service driver at EL1 to query QSPI device info from FW
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_RSU_GET_DEVICE_INFO
 * a1-7 not used
 *
 * Return status
 * a0 INTEL_SIP_SMC_STATUS_OK
 * a1 erasesize0 | size0
 * a2 erasesize1 | size1
 * a3 erasesize2 | size2
 * a4 erasesize3 | size3
 * Or
 *
 * a0 INTEL_SIP_SMC_RSU_ERROR
 */
#define INTEL_SIP_SMC_FUNCID_RSU_GET_DEVICE_INFO 22
#define INTEL_SIP_SMC_RSU_GET_DEVICE_INFO \
	INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_RSU_GET_DEVICE_INFO)

/**
 * Request INTEL_SIP_SMC_SERVICE_COMPLETED
 * Sync call to check if the secure world have completed service request
 * or not.
 *
 * Call register usage:
 * a0: INTEL_SIP_SMC_SERVICE_COMPLETED
 * a1: this register is optional. If used, it is the physical address for
 *     secure firmware to put output data
 * a2: this register is optional. If used, it is the size of output data
 * a3: this register is optional. Set to 0x00004F4E for asynchronous mode
 * a4-a7: not used
 *
 * Return status:
 * a0: INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_ERROR,
 *     INTEL_SIP_SMC_REJECTED or INTEL_SIP_SMC_STATUS_BUSY
 * a1: mailbox error if a0 is INTEL_SIP_SMC_STATUS_ERROR
 * a2: physical address containing the process info
 *     for FCS certificate -- the data contains the certificate status
 *     for FCS cryption -- the data contains the actual data size FW processes
 * a3: output data size
 */
#define INTEL_SIP_SMC_FUNCID_SERVICE_COMPLETED 30
#define INTEL_SIP_SMC_SERVICE_COMPLETED \
	INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_SERVICE_COMPLETED)

/**
 * Request INTEL_SIP_SMC_FIRMWARE_VERSION
 *
 * Sync call used to query the version of running firmware
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FIRMWARE_VERSION
 * a1-a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK or INTEL_SIP_SMC_STATUS_ERROR
 * a1 running firmware version
 */
#define INTEL_SIP_SMC_FUNCID_FIRMWARE_VERSION 31
#define INTEL_SIP_SMC_FIRMWARE_VERSION \
        INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_FIRMWARE_VERSION)

/**
 * SMC call protocol for Mailbox, starting FUNCID from 60
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_MBOX_SEND_CMD
 * a1 mailbox command code
 * a2 physical address that contain mailbox command data (not include header)
 * a3 mailbox command data size in word
 * a4 set to 0 for CASUAL, set to 1 for URGENT
 * a5 physical address for secure firmware to put response data
 *    (not include header)
 * a6 maximum size in word of physical address to store response data
 * a7 not used
 *
 * Return status
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_REJECTED or
 *    INTEL_SIP_SMC_STATUS_ERROR
 * a1 mailbox error code
 * a2 response data length in word
 * a3 not used
 */
#define INTEL_SIP_SMC_FUNCID_MBOX_SEND_CMD 60
	#define INTEL_SIP_SMC_MBOX_SEND_CMD \
	INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_MBOX_SEND_CMD)

/**
 * SMC call protocol for FPGA Crypto Service (FCS)
 * FUNCID starts from 90
 */

/**
 * Request INTEL_SIP_SMC_FCS_RANDOM_NUMBER
 *
 * Sync call used to query the random number generated by the firmware
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_RANDOM_NUMBER
 * a1 the physical address for firmware to write generated random data
 * a2-a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_FCS_ERROR or
 *      INTEL_SIP_SMC_FCS_REJECTED
 * a1 mailbox error
 * a2 the physical address of generated random number
 * a3 size
 */
#define INTEL_SIP_SMC_FUNCID_FCS_RANDOM_NUMBER 90
#define INTEL_SIP_SMC_FCS_RANDOM_NUMBER \
	INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_RANDOM_NUMBER)

/**
 * Request INTEL_SIP_SMC_FCS_CRYPTION
 * Async call for data encryption and HMAC signature generation, or for
 * data decryption and HMAC verification.
 *
 * Call INTEL_SIP_SMC_SERVICE_COMPLETED to get the output encrypted or
 * decrypted data
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_CRYPTION
 * a1 cryption mode (1 for encryption and 0 for decryption)
 * a2 physical address which stores to be encrypted or decrypted data
 * a3 input data size
 * a4 physical address which will hold the encrypted or decrypted output data
 * a5 output data size
 * a6-a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_ERROR or
 *      INTEL_SIP_SMC_STATUS_REJECTED
 * a1-3 not used
 */
#define INTEL_SIP_SMC_FUNCID_FCS_CRYPTION 91
#define INTEL_SIP_SMC_FCS_CRYPTION \
	INTEL_SIP_SMC_STD_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_CRYPTION)

/**
 * Request INTEL_SIP_SMC_FCS_SERVICE_REQUEST
 * Async call for authentication service of HPS software
 *
 * Sync call used by service driver at EL1 to query DCMF (Decision
 * Configuration Management Firmware) version from FW
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_SERVICE_REQUEST
 * a1 the physical address of data block
 * a2 size of data block
 * a3-a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_ERROR or
 *      INTEL_SIP_SMC_REJECTED
 * a1-a3 not used
 */
#define INTEL_SIP_SMC_FUNCID_FCS_SERVICE_REQUEST 92
#define INTEL_SIP_SMC_FCS_SERVICE_REQUEST \
	INTEL_SIP_SMC_STD_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_SERVICE_REQUEST)

/**
 * Request INTEL_SIP_SMC_FUNCID_FCS_SEND_CERTIFICATE
 * Async call to send a signed certificate
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_SEND_CERTIFICATE
 * a1 the physical address of CERTIFICATE block
 * a2 size of data block
 * a3-a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK or INTEL_SIP_SMC_REJECTED
 * a1-a3 not used
 */
#define INTEL_SIP_SMC_FUNCID_FCS_SEND_CERTIFICATE 93
#define INTEL_SIP_SMC_FCS_SEND_CERTIFICATE \
	INTEL_SIP_SMC_STD_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_SEND_CERTIFICATE)

/**
 * Request INTEL_SIP_SMC_FCS_GET_PROVISION_DATA
 * Sync call to dump all the fuses and key hashes
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_GET_PROVISION_DATA
 * a1-a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_ERROR or
 *	INTEL_SIP_SMC_STATUS_REJECTED
 * a1-a3 not used
 *
 */
#define INTEL_SIP_SMC_FUNCID_RSU_DCMF_VERSION 16
#define INTEL_SIP_SMC_RSU_DCMF_VERSION \
	INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_RSU_DCMF_VERSION)

/**
 * Request INTEL_SIP_SMC_RSU_MAX_RETRY
 *
 * Sync call used by service driver at EL1 to query max retry value from FW
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_RSU_MAX_RETRY
 * a1-7 not used
 *
 * Return status
 * a0 INTEL_SIP_SMC_STATUS_OK
 * a1 max retry value
 *
 * Or
 * a0 INTEL_SIP_SMC_RSU_ERROR
 */
#define INTEL_SIP_SMC_FUNCID_RSU_MAX_RETRY 18
#define INTEL_SIP_SMC_RSU_MAX_RETRY \
	INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_RSU_MAX_RETRY)

/**
 * Request INTEL_SIP_SMC_RSU_DCMF_STATUS
 *
 * Sync call used by service driver at EL1 to query DCMF status from FW
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_RSU_DCMF_STATUS
 * a1-7 not used
 *
 * Return status
 * a0 INTEL_SIP_SMC_STATUS_OK
 * a1 dcmf3 | dcmf2 | dcmf1 | dcmf0
 *
 * Or
 *
 * a0 INTEL_SIP_SMC_RSU_ERROR
 */
#define INTEL_SIP_SMC_FUNCID_RSU_DCMF_STATUS 20
#define INTEL_SIP_SMC_RSU_DCMF_STATUS \
	INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_RSU_DCMF_STATUS)

/**
 * Request INTEL_SIP_SMC_SERVICE_COMPLETED
 * Sync call to check if the secure world have completed service request
 * or not.
 *
 * Call register usage:
 * a0: INTEL_SIP_SMC_SERVICE_COMPLETED
 * a1: this register is optional. If used, it is the physical address for
 *     secure firmware to put output data
 * a2: this register is optional. If used, it is the size of output data
 * a3-a7: not used
 *
 * Return status:
 * a0: INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_ERROR,
 *     INTEL_SIP_SMC_REJECTED or INTEL_SIP_SMC_STATUS_BUSY
 * a1: mailbox error if a0 is INTEL_SIP_SMC_STATUS_ERROR
 * a2: physical address containing the process info
 *     for FCS certificate -- the data contains the certificate status
 *     for FCS cryption -- the data contains the actual data size FW processes
 * a3: output data size
 */
#define INTEL_SIP_SMC_FUNCID_SERVICE_COMPLETED 30
#define INTEL_SIP_SMC_SERVICE_COMPLETED \
	INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_SERVICE_COMPLETED)

/**
 * Request INTEL_SIP_SMC_FIRMWARE_VERSION
 *
 * Sync call used to query the version of running firmware
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FIRMWARE_VERSION
 * a1-a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK or INTEL_SIP_SMC_STATUS_ERROR
 * a1 running firmware version
 */
#define INTEL_SIP_SMC_FUNCID_FIRMWARE_VERSION 31
#define INTEL_SIP_SMC_FIRMWARE_VERSION \
        INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_FIRMWARE_VERSION)

/**
 * Request INTEL_SIP_SMC_SVC_VERSION
 *
 * Sync call used to query the SIP SMC API Version
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_SVC_VERSION
 * a1-a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK
 * a1 Major
 * a2 Minor
 */
#define INTEL_SIP_SMC_SVC_FUNCID_VERSION 512
#define INTEL_SIP_SMC_SVC_VERSION \
	INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_SVC_FUNCID_VERSION)

/**
 * SMC call protocol for FPGA Crypto Service (FCS)
 * FUNCID starts from 90
 */

/**
 * Request INTEL_SIP_SMC_FCS_RANDOM_NUMBER
 *
 * Sync call used to query the random number generated by the firmware
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_RANDOM_NUMBER
 * a1 the physical address for firmware to write generated random data
 * a2-a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_FCS_ERROR or
 *      INTEL_SIP_SMC_FCS_REJECTED
 * a1 mailbox error
 * a2 the physical address of generated random number
 * a3 size
 */
#define INTEL_SIP_SMC_FUNCID_FCS_RANDOM_NUMBER 90
#define INTEL_SIP_SMC_FCS_RANDOM_NUMBER \
	INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_RANDOM_NUMBER)

/**
 * Request INTEL_SIP_SMC_FCS_CRYPTION
 * Async call for data encryption and HMAC signature generation, or for
 * data decryption and HMAC verification.
 *
 * Call INTEL_SIP_SMC_SERVICE_COMPLETED to get the output encrypted or
 * decrypted data
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_CRYPTION
 * a1 cryption mode (1 for encryption and 0 for decryption)
 * a2 physical address which stores to be encrypted or decrypted data
 * a3 input data size
 * a4 physical address which will hold the encrypted or decrypted output data
 * a5 output data size
 * a6-a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_ERROR or
 *      INTEL_SIP_SMC_STATUS_REJECTED
 * a1-3 not used
 */
#define INTEL_SIP_SMC_FUNCID_FCS_CRYPTION 91
#define INTEL_SIP_SMC_FCS_CRYPTION \
	INTEL_SIP_SMC_STD_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_CRYPTION)

/**
 * Request INTEL_SIP_SMC_FCS_SERVICE_REQUEST
 * Async call for authentication service of HPS software
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_SERVICE_REQUEST
 * a1 the physical address of data block
 * a2 size of data block
 * a3-a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_ERROR or
 *      INTEL_SIP_SMC_REJECTED
 * a1-a3 not used
 */
#define INTEL_SIP_SMC_FUNCID_FCS_SERVICE_REQUEST 92
#define INTEL_SIP_SMC_FCS_SERVICE_REQUEST \
	INTEL_SIP_SMC_STD_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_SERVICE_REQUEST)

/**
 * Request INTEL_SIP_SMC_FUNCID_FCS_SEND_CERTIFICATE
 * Sync call to send a signed certificate
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_SEND_CERTIFICATE
 * a1 the physical address of CERTIFICATE block
 * a2 size of data block
 * a3-a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK or INTEL_SIP_SMC_FCS_REJECTED
 * a1-a3 not used
 */
#define INTEL_SIP_SMC_FUNCID_FCS_SEND_CERTIFICATE 93
#define INTEL_SIP_SMC_FCS_SEND_CERTIFICATE \
	INTEL_SIP_SMC_STD_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_SEND_CERTIFICATE)

/**
 * Request INTEL_SIP_SMC_FCS_GET_PROVISION_DATA
 * Sync call to dump all the fuses and key hashes
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_GET_PROVISION_DATA
 * a1 the physical address for firmware to write structure of fuse and
 *    key hashes
 * a2-a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_FCS_ERROR or
 *      INTEL_SIP_SMC_FCS_REJECTED
 * a1 mailbox error
 * a2 physical address for the structure of fuse and key hashes
 * a3 the size of structure
 *
 */
#define INTEL_SIP_SMC_FUNCID_FCS_GET_PROVISION_DATA 94
#define INTEL_SIP_SMC_FCS_GET_PROVISION_DATA \
	INTEL_SIP_SMC_STD_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_GET_PROVISION_DATA)

/**
 * Request INTEL_SIP_SMC_FCS_COUNTER_SET_PREAUTHORIZED
 * Sync call to update counter value w/o signed certificate
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_COUNTER_SET_PREAUTHORIZED
 * a1 counter type
 * a2 counter value
 * a3 test bit
 * a3-a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK or INTEL_SIP_SMC_STATUS_ERROR
 * a1-a4 not used
 */
#define INTEL_SIP_SMC_FUNCID_FCS_COUNTER_SET_PREAUTHORIZED 95
#define INTEL_SIP_SMC_FCS_COUNTER_SET_PREAUTHORIZED \
	INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_COUNTER_SET_PREAUTHORIZED)

/**
 * Request INTEL_SIP_SMC_FCS_PSGSIGMA_TEARDOWN
 * Sync call to tear down all previous black key provision sessions and to
 * delete keys assicated with those sessions
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_PSGSIGMA_TEARDOWN
 * a1 the session ID
 * a2-a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_ERROR or
 *    INTEL_SIP_SMC_STATUS_REJECTED
 * a1 mailbox error if a0 is INTEL_SIP_SMC_STATUS_ERROR,
 *    not used if a0 is INTEL_SIP_SMC_STATUS_OK or
 *    INTEL_SIP_SMC_STATUS_REJECTED
 * a2-a3 not used
 */
#define INTEL_SIP_SMC_FUNCID_FCS_PSGSIGMA_TEARDOWN 100
#define INTEL_SIP_SMC_FCS_PSGSIGMA_TEARDOWN \
	INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_PSGSIGMA_TEARDOWN)

/**
 * Request INTEL_SIP_SMC_FCS_CHIP_ID
 * Sync call to get the device ID
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_CHIP_ID
 * a1-a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_ERROR or
 *    INTEL_SIP_SMC_STATUS_REJECTED
 * a1 mailbox error if a0 is INTEL_SIP_SMC_STATUS_ERROR
 * a2 retrieved chipID value low 32 bits
 * a3 retrieved chipID value high 32 bits
 */
#define INTEL_SIP_SMC_FUNCID_FCS_CHIP_ID 101
#define INTEL_SIP_SMC_FCS_CHIP_ID \
	INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_CHIP_ID)

/**
 * Request INTEL_SIP_SMC_FCS_ATTESTATION_SUBKEY
 * Sync call to the device attestation subkey
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_ATTESTATION_SUBKEY
 * a1 physical address of subkey command data
 * a2 subkey command data size
 * a3 physical address of to be filled subkey response data
 * a4 subkey response data size
 * a5-a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, or INTEL_SIP_SMC_STATUS_ERROR
 * a1 mailbox error if a0 is INTEL_SIP_SMC_STATUS_ERROR
 * a2 physical address of the filled subkey response data
 * a3 size of the filled subkey response dat
 */
#define INTEL_SIP_SMC_FUNCID_FCS_ATTESTATION_SUBKEY 102
#define INTEL_SIP_SMC_FCS_ATTESTATION_SUBKEY \
	INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_ATTESTATION_SUBKEY)

/**
 * Request INTEL_SIP_SMC_FCS_ATTESTATION_MEASUREMENTS
 * Async call to get device attestation measurements
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_ATTESTATION_MEASUREMENTS
 * a1 physical address of measurement command data
 * a2 measurement command data size
 * a3 physical address of to be filled measurement response data
 * a4 measurement response data size
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, or INTEL_SIP_SMC_STATUS_ERROR
 * a1 mailbox error if a0 is INTEL_SIP_SMC_STATUS_ERROR
 * a2 physical address of the filled subkey measurement data
 * a3 size of the filled subkey measurement data
 */
#define INTEL_SIP_SMC_FUNCID_FCS_ATTESTATION_MEASUREMENTS 103
#define INTEL_SIP_SMC_FCS_ATTESTATION_MEASUREMENTS \
	INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_ATTESTATION_MEASUREMENTS)

/**
 * Request INTEL_SIP_SMC_HWMON_READTEMP
 * Sync call to request temperature
 *
 * Call register usage:
 * a0 Temperature Channel
 * a1-a7 not used
 *
 * Return status
 * a0 INTEL_SIP_SMC_STATUS_OK
 * a1 Temperature Value
 * a2-a3 not used
 */
#define INTEL_SIP_SMC_FUNCID_HWMON_READTEMP 32
#define INTEL_SIP_SMC_HWMON_READTEMP \
	INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_HWMON_READTEMP)

/**
 * Request INTEL_SIP_SMC_HWMON_READVOLT
 * Sync call to request voltage
 *
 * Call register usage:
 * a0 Voltage Channel
 * a1-a7 not used
 *
 * Return status
 * a0 INTEL_SIP_SMC_STATUS_OK
 * a1 Voltage Value
 * a2-a3 not used
 */
#define INTEL_SIP_SMC_FUNCID_HWMON_READVOLT 33
#define INTEL_SIP_SMC_HWMON_READVOLT \
	INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_HWMON_READVOLT)

/**
 * Request INTEL_SIP_SMC_FCS_GET_ATTESTATION_CERTIFICATE
 * Sync call to get device attestation certificate
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_GET_ATTESTATION_CERTIFICATE
 * a1 the type of certificate request
 * a2 the physical address which holds certificate response data
 * a3 the size of the certificate response data
 * a4-a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_NOT_SUPPORTED or
 *    INTEL_SIP_SMC_STATUS_ERROR
 * a1 mailbox error if a0 is INTEL_SIP_SMC_STATUS_ERROR
 * a2 physical address of the requested certificate
 * a3 sized of the requested certificate
 */
#define INTEL_SIP_SMC_FUNCID_FCS_GET_ATTESTATION_CERTIFICATE 104
#define INTEL_SIP_SMC_FCS_GET_ATTESTATION_CERTIFICATE \
	INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_GET_ATTESTATION_CERTIFICATE)

/**
 * Request INTEL_SIP_SMC_FCS_CREATE_CERTIFICATE_ON_RELOAD
 * Sync call to specify what certificate is to be generated
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_CREATE_CERTIFICATE_ON_RELOAD
 * a1 the type of certificat request
 * a2-a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_NOT_SUPPORTED or
 *    INTEL_SIP_SMC_STATUS_ERROR
 * a1 mailbox error if a0 is INTEL_SIP_SMC_STATUS_ERROR
 * a2-a3 not used
 */
#define INTEL_SIP_SMC_FUNCID_FCS_CREATE_CERTIFICATE_ON_RELOAD 105
#define INTEL_SIP_SMC_FCS_CREATE_CERTIFICATE_ON_RELOAD \
	INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_CREATE_CERTIFICATE_ON_RELOAD)

/**
 * Request INTEL_SIP_SMC_FCS_GET_ROM_PATCH_SHA384
 *
 * Sync call used to dump the SHA384 hash of the rom patch
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_GET_ROM_PATCH_SHA384
 * a1 the physical address for firmware to write generated SHA384 data
 * a2-a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_FCS_ERROR or
 *      INTEL_SIP_SMC_FCS_REJECTED
 * a1 mailbox error
 * a2 the physical address of the SHA384 checksum
 * a3 size of the SHA384 checksum
 */
#define INTEL_SIP_SMC_FUNCID_FCS_GET_ROM_PATCH_SHA384 64
#define INTEL_SIP_SMC_FCS_GET_ROM_PATCH_SHA384 \
	INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_GET_ROM_PATCH_SHA384)

/**
 * Request INTEL_SIP_SMC_FCS_OPEN_CRYPTO_SERVICE_SESSION
 * Sync call to open and establish a crypto service session with firmware
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_OPEN_CRYPTO_SERVICE_SESSION
 * a1-a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_NOT_SUPPORTED or
 *    INTEL_SIP_SMC_STATUS_ERROR
 * a1 mailbox error if a0 is INTEL_SIP_SMC_STATUS_ERROR
 * a2 session ID
 * a3 not used
 */
#define INTEL_SIP_SMC_FUNCID_FCS_OPEN_CRYPTO_SERVICE_SESSION 110
#define INTEL_SIP_SMC_FCS_OPEN_CRYPTO_SERVICE_SESSION \
        INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_OPEN_CRYPTO_SERVICE_SESSION)

/**
 * Request INTEL_SIP_SMC_FCS_CLOSE_CRYPTO_SERVICE_SESSION
 * Sync call to close a service session
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_CLOSE_CRYPTO_SERVICE_SESSION
 * a1 session ID
 * a2-a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_NOT_SUPPORTED or
 *    INTEL_SIP_SMC_STATUS_ERROR
 * a1 mailbox error if a0 is INTEL_SIP_SMC_STATUS_ERROR
 * a2-a3 not used
 */
#define INTEL_SIP_SMC_FUNCID_FCS_CLOSE_CRYPTO_SERVICE_SESSION 111
#define INTEL_SIP_SMC_FCS_CLOSE_CRYPTO_SERVICE_SESSION \
        INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_CLOSE_CRYPTO_SERVICE_SESSION)

/**
 * Request INTEL_SIP_SMC_FCS_IMPORT_CRYPTO_SERVICE_KEY
 * Async call to import crypto service key to the device
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_IMPORT_CRYPTO_SERVICE_KEY
 * a1 physical address of the service key object with header
 * a3 size of the service key object
 * a4-a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_ERROR or
 *      INTEL_SIP_SMC_STATUS_REJECTED
 * a1-3 not used
 */
#define INTEL_SIP_SMC_FUNCID_FCS_IMPORT_CRYPTO_SERVICE_KEY 112
#define INTEL_SIP_SMC_FCS_IMPORT_CRYPTO_SERVICE_KEY \
	INTEL_SIP_SMC_STD_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_IMPORT_CRYPTO_SERVICE_KEY)

/**
 * Request INTEL_SIP_SMC_FCS_EXPORT_CRYPTO_SERVICE_KEY
 * Sync call to export crypto service key from the device
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_EXPORT_CRYPTO_SERVICE_KEY
 * a1 session ID
 * a2 key UID
 * a3 physical address of the exported service key object
 * a4 size of the exported service key object, max is (88 words + 3 header words)
 * a5-a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_NOT_SUPPORTED or
 *    INTEL_SIP_SMC_STATUS_ERROR
 * a1 mailbox and status errors if a0 is INTEL_SIP_SMC_STATUS_ERROR
 *      31:24 -- reserved
 *      23:16 -- import/export/removal status error
 *      15:11 -- reserved
 *      10:0  -- mailbox error
 * a2 physical address of the exported service key object
 * a3 size of the exported service key object
 */
#define INTEL_SIP_SMC_FUNCID_FCS_EXPORT_CRYPTO_SERVICE_KEY 113
#define INTEL_SIP_SMC_FCS_EXPORT_CRYPTO_SERVICE_KEY \
        INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_EXPORT_CRYPTO_SERVICE_KEY)

/**
 * Request INTEL_SIP_SMC_FCS_REMOVE_CRYPTO_SERVICE_KEY
 * Sync call to remove the crypto service kers from the device
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_REMOVE_CRYPTO_SERVICE_KEY
 * a1 session ID
 * a2 key UID
 * a3-a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_NOT_SUPPORTED or
 *    INTEL_SIP_SMC_STATUS_ERROR
 * a1 mailbox and status errors if a0 is INTEL_SIP_SMC_STATUS_ERROR
 * 	31:24 -- reserved
 * 	23:16 -- import/export/removal status error
 * 	15:11 -- reserved
 * 	10:0  -- mailbox error
 * a2-a3 not used
 */
#define INTEL_SIP_SMC_FUNCID_FCS_REMOVE_CRYPTO_SERVICE_KEY 114
#define INTEL_SIP_SMC_FCS_REMOVE_CRYPTO_SERVICE_KEY \
	INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_REMOVE_CRYPTO_SERVICE_KEY)

/**
 * Request INTEL_SIP_SMC_FCS_GET_CRYPTO_SERVICE_KEY_INFO
 * Sync call to query the crypto service keys on the device
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_GET_CRYPTO_SERVICE_KEY_INFO
 * a1 session ID
 * a2 key UID
 * a3 physical address of the reponse data
 * a4 max size of the response data (36 words with header)
 * a3-a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_NOT_SUPPORTED or
 *    INTEL_SIP_SMC_STATUS_ERROR
 * a1 mailbox and status errors if a0 is INTEL_SIP_SMC_STATUS_ERROR
 * 	31:24 -- reserved
 * 	23:16 -- import/export/removal status error
 * 	15:11 -- reserved
 * 	10:0  -- mailbox error
 * a2 physical address of the reponse data
 * a3 size of the response data
 */
#define INTEL_SIP_SMC_FUNCID_FCS_GET_CRYPTO_SERVICE_KEY_INFO 115
#define INTEL_SIP_SMC_FCS_GET_CRYPTO_SERVICE_KEY_INFO \
	INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_GET_CRYPTO_SERVICE_KEY_INFO)

/**
 * Request INTEL_SIP_SMC_FCS_AES_CRYPTO_INIT
 * Sync call to initialize AES crypto operation
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_AES_CRYPTO_INIT
 * a1 session ID
 * a2 context ID
 * a3 key UID
 * a4 physical address of AES crypto parameter data (include block mode,
 *    encrypt/decrypt, IV fields
 * a5 size of of AES crypto parameter data
 * a6-a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_NOT_SUPPORTED or
 *    INTEL_SIP_SMC_STATUS_ERROR
 * a1 mailbox errors if a0 is INTEL_SIP_SMC_STATUS_ERROR
 * a2-a3 not used
 */
#define INTEL_SIP_SMC_FUNCID_FCS_AES_CRYPTO_INIT 116
#define INTEL_SIP_SMC_FCS_AES_CRYPTO_INIT \
        INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_AES_CRYPTO_INIT)

/**
 * Request INTEL_SIP_SMC_FCS_AES_CRYPTO_UPDATE
 * Sync call to decrypt/encrypt a data block
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_AES_CRYPTO_UPDATE
 * a1 session ID
 * a2 context ID
 * a3 physical address of source
 * a4 size of source
 * a5 physical address of destination
 * a6 size of destination
 * a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_NOT_SUPPORTED or
 *    INTEL_SIP_SMC_STATUS_ERROR
 * a1-a3 not used
 */
#define INTEL_SIP_SMC_FUNCID_FCS_AES_CRYPTO_UPDATE 117
#define INTEL_SIP_SMC_FCS_AES_CRYPTO_UPDATE \
        INTEL_SIP_SMC_STD_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_AES_CRYPTO_UPDATE)

/**
 * Request INTEL_SIP_SMC_FCS_AES_CRYPTO_FINALIZE
 * Sync call to decrypt/encrypt a data block
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_AES_CRYPTO_FINALIZE
 * a1 session ID
 * a2 context ID
 * a3 physical address of source
 * a4 size of source
 * a5 physical address of destation
 * a6 size of destation
 * a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_NOT_SUPPORTED or
 *    INTEL_SIP_SMC_STATUS_ERROR
 * a1-a3 not used
 */
#define INTEL_SIP_SMC_FUNCID_FCS_AES_CRYPTO_FINALIZE 118
#define INTEL_SIP_SMC_FCS_AES_CRYPTO_FINALIZE \
        INTEL_SIP_SMC_STD_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_AES_CRYPTO_FINALIZE)

/**
 * Request INTEL_SIP_SMC_FCS_GET_DIGEST_INIT
 * Sync call to request the SHA-2 hash digest on a blob
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_GET_DIGEST_INIT
 * a1 session ID
 * a2 context ID
 * a3 key UID
 * a4 size of crypto parameter data
 * a5 the crypto parameter
 *      3:0     SHA opeation mode
 *      7:4     digist size
 *      63:8    not used
 * a6-a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_NOT_SUPPORTED or
 *    INTEL_SIP_SMC_STATUS_ERROR
 * a1 mailbox errors if a0 is INTEL_SIP_SMC_STATUS_ERROR
 * a2-a3 not used
 */
#define INTEL_SIP_SMC_FUNCID_FCS_GET_DIGEST_INIT 119
#define INTEL_SIP_SMC_FCS_GET_DIGEST_INIT \
        INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_GET_DIGEST_INIT)

/**
 * Request INTEL_SIP_SMC_FCS_GET_DIGEST_UPDATE
 * Sync call to request the SHA-2 hash digest on a blob
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_GET_DIGEST_UPDATE
 * a1 session ID
 * a2 context ID
 * a3 physical address of source
 * a4 size of source
 * a5 physical address of destination
 * a6 size of destination
 * a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_NOT_SUPPORTED or
 *    INTEL_SIP_SMC_STATUS_ERROR
 * a1 mailbox errors if a0 is INTEL_SIP_SMC_STATUS_ERROR
 * a2 physical address of response data
 * a3 size of response data
 */
#define INTEL_SIP_SMC_FUNCID_FCS_GET_DIGEST_UPDATE 120
#define INTEL_SIP_SMC_FCS_GET_DIGEST_UPDATE \
        INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_GET_DIGEST_UPDATE)

/**
 * Request INTEL_SIP_SMC_FCS_GET_DIGEST_FINALIZE
 * Sync call to request the SHA-2 hash digest on a blob
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_GET_DIGEST_FINALIZE
 * a1 session ID
 * a2 context ID
 * a3 physical address of source
 * a4 size of source
 * a5 physical address of destation
 * a6 size of destation
 * a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_NOT_SUPPORTED or
 *    INTEL_SIP_SMC_STATUS_ERROR
 * a1 mailbox errors if a0 is INTEL_SIP_SMC_STATUS_ERROR
 * a2 physical address of response data
 * a3 size of response data
 */
#define INTEL_SIP_SMC_FUNCID_FCS_GET_DIGEST_FINALIZE 121
#define INTEL_SIP_SMC_FCS_GET_DIGEST_FINALIZE \
        INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_GET_DIGEST_FINALIZE)

/**
 * Request INTEL_SIP_SMC_FCS_MAC_VERIFY_INIT
 * Sync call to check the integrity and authenticity of a blob by comparing
 * the calculated MAC with tagged MAC
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_MAC_VERIFY_INIT
 * a1 session ID
 * a2 context ID
 * a3 key UID
 * a4 size of crypto parameter data
 * a5 crypto parameter data
 * 	3:0	not used
 *      7:4     digist size
 *      63:8    not used
 * a6-a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_NOT_SUPPORTED or
 *    INTEL_SIP_SMC_STATUS_ERROR
 * a1 mailbox errors if a0 is INTEL_SIP_SMC_STATUS_ERROR
 * a2 physical address of response data
 * a3 size of response data
 */
#define INTEL_SIP_SMC_FUNCID_FCS_MAC_VERIFY_INIT 122
#define INTEL_SIP_SMC_FCS_MAC_VERIFY_INIT \
        INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_MAC_VERIFY_INIT)

/**
 * Request INTEL_SIP_SMC_FCS_MAC_VERIFY_UPDATE
 * Sync call to check the integrity and authenticity of a blob by comparing
 * the calculated MAC with tagged MAC
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_MAC_VERIFY_UPDATE
 * a1 session ID
 * a2 context ID
 * a3 physical address of source
 * a4 size of source
 * a5 physical address of destination
 * a6 size of destination
 * a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_NOT_SUPPORTED or
 *    INTEL_SIP_SMC_STATUS_ERROR
 * a1 mailbox errors if a0 is INTEL_SIP_SMC_STATUS_ERROR
 * a2 physical address of response data
 * a3 size of response data
 */
#define INTEL_SIP_SMC_FUNCID_FCS_MAC_VERIFY_UPDATE 123
#define INTEL_SIP_SMC_FCS_MAC_VERIFY_UPDATE \
        INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_MAC_VERIFY_UPDATE)

/**
 * Request INTEL_SIP_SMC_FCS_MAC_VERIFY_FINALIZE
 * Sync call to check the integrity and authenticity of a blob by comparing
 * the calculated MAC with tagged MAC
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_MAC_VERIFY_FINALIZE
 * a1 session ID
 * a2 context ID
 * a3 physical address of source
 * a4 size of source
 * a5 physical address of destation
 * a6 size of destation
 * a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_NOT_SUPPORTED or
 *    INTEL_SIP_SMC_STATUS_ERROR
 * a1 mailbox errors if a0 is INTEL_SIP_SMC_STATUS_ERROR
 * a2 physical address of response data
 * a3 size of response data
 */
#define INTEL_SIP_SMC_FUNCID_FCS_MAC_VERIFY_FINALIZE 124
#define INTEL_SIP_SMC_FCS_MAC_VERIFY_FINALIZE \
        INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_MAC_VERIFY_FINALIZE)

/**
 * Request INTEL_SIP_SMC_FCS_ECDSA_HASH_SIGNING_INIT
 * Sync call to sends digital signature signing request on a data blob
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_ECDSA_HASH_SIGNING_INIT
 * a1 session ID
 * a2 context ID
 * a3 key UID
 * a4 size of crypto parameter data
 * a5 size of crypto parameter data
 *      3:0     ECC algoritim
 *      63:4    not used
 * a6-a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_NOT_SUPPORTED or
 *    INTEL_SIP_SMC_STATUS_ERROR
 * a1 mailbox errors if a0 is INTEL_SIP_SMC_STATUS_ERROR
 * a2-a3 not used
 */
#define INTEL_SIP_SMC_FUNCID_FCS_ECDSA_HASH_SIGNING_INIT 125
#define INTEL_SIP_SMC_FCS_ECDSA_HASH_SIGNING_INIT \
        INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_ECDSA_HASH_SIGNING_INIT)

/**
 * Request INTEL_SIP_SMC_FCS_ECDSA_HASH_SIGNING_FINALIZE
 * Sync call to sends digital signature signing request on a data blob
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_ECDSA_HASH_SIGNING_FINALIZE
 * a1 session ID
 * a2 context ID
 * a3 physical address of source
 * a4 size of source
 * a5 physical address of destation
 * a6 size of destation
 * a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_NOT_SUPPORTED or
 *    INTEL_SIP_SMC_STATUS_ERROR
 * a1 mailbox errors if a0 is INTEL_SIP_SMC_STATUS_ERROR
 * a2 physical address of response data
 * a3 size of response data
 */
#define INTEL_SIP_SMC_FUNCID_ECDSA_HASH_SIGNING_FINALIZE 127
#define INTEL_SIP_SMC_FCS_ECDSA_HASH_SIGNING_FINALIZE \
        INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_ECDSA_HASH_SIGNING_FINALIZE)

/**
 * Request INTEL_SIP_SMC_FCS_ECDSA_SHA2_DATA_SIGNING_INIT
 * Sync call to digital signature signing request on a data blob
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_ECDSA_SHA2_DATA_SIGNING_INIT
 * a1 session ID
 * a2 context ID
 * a3 key UID
 * a4 size of crypto parameter data
 * a5 crypto parameter data
 *      3:0     ECC algorithm
 *      63:4    not used
 * a6-a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_NOT_SUPPORTED or
 *    INTEL_SIP_SMC_STATUS_ERROR
 * a1 mailbox errors if a0 is INTEL_SIP_SMC_STATUS_ERROR
 * a2-a3 not used
 */
#define INTEL_SIP_SMC_FUNCID_ECDSA_SHA2_DATA_SIGNING_INIT 128
#define INTEL_SIP_SMC_FCS_ECDSA_SHA2_DATA_SIGNING_INIT \
        INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_ECDSA_SHA2_DATA_SIGNING_INIT)

/**
 * Request INTEL_SIP_SMC_FCS_ECDSA_SHA2_DATA_SIGNING_UPDATE
 * Sync call to digital signature signing request on a data blob
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_ECDSA_SHA2_DATA_SIGNING_UPDATE
 * a1 session ID
 * a2 context ID
 * a3 physical address of source
 * a4 size of source
 * a5 physical address of destination
 * a6 size of destination
 * a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_NOT_SUPPORTED or
 *    INTEL_SIP_SMC_STATUS_ERROR
 * a1 mailbox errors if a0 is INTEL_SIP_SMC_STATUS_ERROR
 * a2 physical address of response data
 * a3 size of response data
 */
#define INTEL_SIP_SMC_FUNCID_ECDSA_SHA2_DATA_SIGNING_UPDATE 129
#define INTEL_SIP_SMC_FCS_ECDSA_SHA2_DATA_SIGNING_UPDATE \
        INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_ECDSA_SHA2_DATA_SIGNING_UPDATE)

/**
 * Request INTEL_SIP_SMC_FCS_ECDSA_SHA2_DATA_SIGNING_FINALIZE
 * Sync call to digital signature signing request on a data blob
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_ECDSA_SHA2_DATA_SIGNING_FINALIZE
 * a1 session ID
 * a2 context ID
 * a3 physical address of source
 * a4 size of source
 * a5 physical address of destation
 * a6 size of destation
 * a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_NOT_SUPPORTED or
 *    INTEL_SIP_SMC_STATUS_ERROR
 * a1 mailbox errors if a0 is INTEL_SIP_SMC_STATUS_ERROR
 * a2 physical address of response data
 * a3 size of response data
 */
#define INTEL_SIP_SMC_FUNCID_ECDSA_SHA2_DATA_SIGNING_FINALIZE 130
#define INTEL_SIP_SMC_FCS_ECDSA_SHA2_DATA_SIGNING_FINALIZE \
        INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_ECDSA_SHA2_DATA_SIGNING_FINALIZE)

/**
 * Request INTEL_SIP_SMC_FCS_ECDSA_HASH_SIGNATURE_VERIFY_INIT
 * Sync call to sends digital signature verify request with precalculated hash
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_ECDSA_HASH_SIGNATURE_VERIFY_INIT
 * a1 session ID
 * a2 context ID
 * a3 key UID
 * a4 size of crypto parameter data
 * a5 crypto parameter data
 *      3:0     ECC algorithm
 *      63:4    not used
 * a6-a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_NOT_SUPPORTED or
 *    INTEL_SIP_SMC_STATUS_ERROR
 * a1 mailbox errors if a0 is INTEL_SIP_SMC_STATUS_ERROR
 * a2-a3 not used
 */
#define INTEL_SIP_SMC_FUNCID_ECDSA_HASH_SIGNATURE_VERIFY_INIT 131
#define INTEL_SIP_SMC_FCS_ECDSA_HASH_SIGNATURE_VERIFY_INIT \
        INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_ECDSA_HASH_SIGNATURE_VERIFY_INIT)

/**
 * Request INTEL_SIP_SMC_FCS_ECDSA_HASH_SIGNATURE_VERIFY_FINALIZE
 * Sync call to sends digital signature verify request with precalculated hash
 *
 * a0 INTEL_SIP_SMC_FCS_ECDSA_HASH_SIGNATURE_VERIFY_FINALIZE
 * a1 session ID
 * a2 context ID
 * a3 physical address of source
 * a4 size of source
 * a5 physical address of destation
 * a6 size of destation
 * a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_NOT_SUPPORTED or
 *    INTEL_SIP_SMC_STATUS_ERROR
 * a1 mailbox errors if a0 is INTEL_SIP_SMC_STATUS_ERROR
 * a2 physical address of response data
 * a3 size of response data
 */
#define INTEL_SIP_SMC_FUNCID_FCS_ECDSA_HASH_SIGNATURE_VERIFY_FINALIZE 133
#define INTEL_SIP_SMC_FCS_ECDSA_HASH_SIGNATURE_VERIFY_FINALIZE \
        INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_ECDSA_HASH_SIGNATURE_VERIFY_FINALIZE)

/**
 * Request INTEL_SIP_SMC_FCS_ECDSA_SHA2_DATA_SIGNATURE_VERIFY_INIT
 * Sync call to send digital signature verify request
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_ECDSA_SHA2_DATA_SIGNATURE_VERIFY_INIT
 * a1 session ID
 * a2 context ID
 * a3 key UID
 * a4 size of crypto parameter data
 * a5 crypto parameter data
 *      3:0     ECC algorithm
 *      63:4    not used
 * a6-a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_NOT_SUPPORTED or
 *    INTEL_SIP_SMC_STATUS_ERROR
 * a1 mailbox errors if a0 is INTEL_SIP_SMC_STATUS_ERROR
 * a2-a3 not used
 */
#define INTEL_SIP_SMC_FUNCID_FCS_ECDSA_SHA2_DATA_SIGNATURE_VERIFY_INIT 134
#define INTEL_SIP_SMC_FCS_ECDSA_SHA2_DATA_SIGNATURE_VERIFY_INIT \
        INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_ECDSA_SHA2_DATA_SIGNATURE_VERIFY_INIT)

/**
 * Request INTEL_SIP_SMC_FCS_ECDSA_SHA2_DATA_SIGNATURE_VERIFY_UPDATE
 * Sync call to send digital signature verify request
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_ECDSA_SHA2_DATA_SIGNATURE_VERIFY_UPDATE
 * a1 session ID
 * a2 context ID
 * a3 physical address of source (contain user data)
 * a4 size of source
 * a5 physical address of destination
 * a6 size of destination
 * a7 size of user data
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_NOT_SUPPORTED or
 *    INTEL_SIP_SMC_STATUS_ERROR
 * a1 mailbox errors if a0 is INTEL_SIP_SMC_STATUS_ERROR
 * a2 physical address of response data
 * a3 size of response data
 */
#define INTEL_SIP_SMC_FUNCID_FCS_ECDSA_SHA2_DATA_SIGNATURE_VERIFY_UPDATE 135
#define INTEL_SIP_SMC_FCS_ECDSA_SHA2_DATA_SIGNATURE_VERIFY_UPDATE \
        INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_ECDSA_SHA2_DATA_SIGNATURE_VERIFY_UPDATE)

/**
 * Request INTEL_SIP_SMC_FCS_ECDSA_SHA2_DATA_SIGNATURE_VERIFY_FINALIZE
 * Sync call to send digital signature verify request
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_ECDSA_SHA2_DATA_SIGNATURE_VERIFY_FINALIZE
 * a1 session ID
 * a2 context ID
 * a3 physical address of source
 * a4 size of source
 * a5 physical address of destation
 * a6 size of destation
 * a7 size of user data
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_NOT_SUPPORTED or
 *    INTEL_SIP_SMC_STATUS_ERROR
 * a1 mailbox errors if a0 is INTEL_SIP_SMC_STATUS_ERROR
 * a2 physical address of response data
 * a3 size of response data
 */
#define INTEL_SIP_SMC_FUNCID_FCS_ECDSA_SHA2_DATA_SIGNATURE_VERIFY_FINALIZE 136
#define INTEL_SIP_SMC_FCS_ECDSA_SHA2_DATA_SIGNATURE_VERIFY_FINALIZE \
        INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_ECDSA_SHA2_DATA_SIGNATURE_VERIFY_FINALIZE)

/**
 * Request INTEL_SIP_SMC_FCS_ECDSA_GET_PUBLIC_KEY_INIT
 * Sync call to send the request to get the public key
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_ECDSA_GET_PUBLIC_KEY_INIT
 * a1 session ID
 * a2 context ID
 * a3 key UID
 * a4 size of crypto parameter data
 * a5 crypto parameter data
 *      3:0     EE algorithm
 *      63:4    not used
 * a6-a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_NOT_SUPPORTED or
 *    INTEL_SIP_SMC_STATUS_ERROR
 * a1 mailbox errors if a0 is INTEL_SIP_SMC_STATUS_ERROR
 * a2-a3 not used
 */
#define INTEL_SIP_SMC_FUNCID_FCS_ECDSA_GET_PUBLIC_KEY_INIT 137
#define INTEL_SIP_SMC_FCS_ECDSA_GET_PUBLIC_KEY_INIT \
        INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_ECDSA_GET_PUBLIC_KEY_INIT)

/**
 * Request INTEL_SIP_SMC_FCS_ECDSA_GET_PUBLIC_KEY_FINALIZE
 * Sync call to send the request to get the public key
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_ECDSA_GET_PUBLIC_KEY_FINALIZE
 * a1 session ID
 * a2 context ID
 * a3 physical address of response data
 * a4 size of response data
 * a5-a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_NOT_SUPPORTED or
 *    INTEL_SIP_SMC_STATUS_ERROR
 * a1 mailbox errors if a0 is INTEL_SIP_SMC_STATUS_ERROR
 * a2 physical address of response data
 * a3 size of response data
 */
#define INTEL_SIP_SMC_FUNCID_FCS_FCS_ECDSA_GET_PUBLIC_KEY_FINALIZE 139
#define INTEL_SIP_SMC_FCS_ECDSA_GET_PUBLIC_KEY_FINALIZE \
        INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_FCS_ECDSA_GET_PUBLIC_KEY_FINALIZE)

/**
 * Request INTEL_SIP_SMC_FCS_ECDH_INIT
 * Sync call to send the request on generating a share secret on
 * Diffie-Hellman key exchange
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_ECDH_INIT
 * a1 session ID
 * a2 context ID
 * a3 key UID
 * a4 size of crypto parameter data
 * a5 crypto parameter data
 *      3:0     ECC algorithm
 *      63:4    not used
 * a6-a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_NOT_SUPPORTED or
 *    INTEL_SIP_SMC_STATUS_ERROR
 * a1 mailbox errors if a0 is INTEL_SIP_SMC_STATUS_ERROR
 * a2-a3 not used
 */
#define INTEL_SIP_SMC_FUNCID_FCS_ECDH_INIT 140
#define INTEL_SIP_SMC_FCS_ECDH_INIT \
        INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_ECDH_INIT)

/**
 * Request INTEL_SIP_SMC_FCS_ECDH_FINALIZE
 * Sync call to send the request on generating a share secret on
 * Diffie-Hellman key exchange
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_ECDH_FINALIZE
 * a1 session ID
 * a2 context ID
 * a3 physical address of source
 * a4 size of source
 * a5 physical address of destation
 * a6 size of destation
 * a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_NOT_SUPPORTED or
 *    INTEL_SIP_SMC_STATUS_ERROR
 * a1 mailbox errors if a0 is INTEL_SIP_SMC_STATUS_ERROR
 * a2 physical address of response data
 * a3 size of response data
 */
#define INTEL_SIP_SMC_FUNCID_FCS_ECDH_FINALIZE 142
#define INTEL_SIP_SMC_FCS_ECDH_FINALIZE \
        INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_ECDH_FINALIZE)

/**
 * Request INTEL_SIP_SMC_FCS_RANDOM_NUMBER_EXT
 *
 * Async call used to query the random number generated by the firmware,
 * the maxim random number is 4080 bytes.
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_RANDOM_NUMBER_EXT
 * a1 session ID
 * a2 context ID
 * a3 size of the requested random data
 * a4-a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_NOT_SUPPORTED or
 *    INTEL_SIP_SMC_STATUS_ERROR
 * a1-a3 not used
 */
#define INTEL_SIP_SMC_FUNCID_FCS_RANDOM_NUMBER_EXT 143
#define INTEL_SIP_SMC_FCS_RANDOM_NUMBER_EXT \
        INTEL_SIP_SMC_STD_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_RANDOM_NUMBER_EXT)

/**
 * Request INTEL_SIP_SMC_FCS_CRYPTION_EXT
 * Sync call for data encryption or data decryption.
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_CRYPTION_EXT
 * a1 session ID
 * a2 context ID
 * a3 cryption operating mode (1 for encryption and 0 for decryption)
 * a4 physical address which stores to be encrypted or decrypted data
 * a5 size of input data
 * a6 physical address which will hold the encrypted or decrypted output data
 * a7 size of output data
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_NOT_SUPPORTED or
 *    INTEL_SIP_SMC_STATUS_ERROR
 * a1 mailbox errors if a0 is INTEL_SIP_SMC_STATUS_ERROR
 * a2 physical address of (output) decrypted or encrypted data
 * a3 size of (output) decrypted or encrypted data
 */
#define INTEL_SIP_SMC_FUNCID_FCS_CRYPTION_EXT 144
#define INTEL_SIP_SMC_FCS_CRYPTION_EXT \
        INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_CRYPTION_EXT)

/**
 * Request INTEL_SIP_SMC_FCS_GET_DIGEST_SMMU_UPDATE
 * Sync call to request the SHA-2 hash digest on a blob
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_GET_DIGEST_SMMU_UPDATE
 * a1 session ID
 * a2 context ID
 * a3 physical address of source
 * a4 size of source
 * a5 physical address of destination
 * a6 size of destination
 * a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_NOT_SUPPORTED or
 *    INTEL_SIP_SMC_STATUS_ERROR
 * a1 mailbox errors if a0 is INTEL_SIP_SMC_STATUS_ERROR
 * a2 physical address of response data
 * a3 size of response data
 */
#define INTEL_SIP_SMC_FUNCID_FCS_GET_DIGEST_SMMU_UPDATE 145
#define INTEL_SIP_SMC_FCS_GET_DIGEST_SMMU_UPDATE \
	INTEL_SIP_SMC_STD_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_GET_DIGEST_SMMU_UPDATE)

/**
 * Request INTEL_SIP_SMC_FCS_GET_DIGEST_SMMU_FINALIZE
 * Sync call to request the SHA-2 hash digest on a blob
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_GET_DIGEST_SMMU_FINALIZE
 * a1 session ID
 * a2 context ID
 * a3 physical address of source
 * a4 size of source
 * a5 physical address of destation
 * a6 size of destation
 * a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_NOT_SUPPORTED or
 *    INTEL_SIP_SMC_STATUS_ERROR
 * a1 mailbox errors if a0 is INTEL_SIP_SMC_STATUS_ERROR
 * a2 physical address of response data
 * a3 size of response data
 */
#define INTEL_SIP_SMC_FUNCID_FCS_GET_DIGEST_SMMU_FINALIZE 146
#define INTEL_SIP_SMC_FCS_GET_DIGEST_SMMU_FINALIZE \
	INTEL_SIP_SMC_STD_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_GET_DIGEST_SMMU_FINALIZE)

/**
 * Request INTEL_SIP_SMC_FCS_MAC_VERIFY_SMMU_UPDATE
 * Sync call to check the integrity and authenticity of a blob by comparing
 * the calculated MAC with tagged MAC
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_MAC_VERIFY_SMMU_UPDATE
 * a1 session ID
 * a2 context ID
 * a3 physical address of source
 * a4 size of source
 * a5 physical address of destination
 * a6 size of destination
 * a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_NOT_SUPPORTED or
 *    INTEL_SIP_SMC_STATUS_ERROR
 * a1 mailbox errors if a0 is INTEL_SIP_SMC_STATUS_ERROR
 * a2 physical address of response data
 * a3 size of response data
 */
#define INTEL_SIP_SMC_FUNCID_FCS_MAC_VERIFY_SMMU_UPDATE 147
#define INTEL_SIP_SMC_FCS_MAC_VERIFY_SMMU_UPDATE \
	INTEL_SIP_SMC_STD_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_MAC_VERIFY_SMMU_UPDATE)

/**
 * Request INTEL_SIP_SMC_FCS_MAC_VERIFY_SMMU_FINALIZE
 * Sync call to check the integrity and authenticity of a blob by comparing
 * the calculated MAC with tagged MAC
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_MAC_VERIFY_SMMU_FINALIZE
 * a1 session ID
 * a2 context ID
 * a3 physical address of source
 * a4 size of source
 * a5 physical address of destation
 * a6 size of destation
 * a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_NOT_SUPPORTED or
 *    INTEL_SIP_SMC_STATUS_ERROR
 * a1 mailbox errors if a0 is INTEL_SIP_SMC_STATUS_ERROR
 * a2 physical address of response data
 * a3 size of response data
 */
#define INTEL_SIP_SMC_FUNCID_FCS_MAC_VERIFY_SMMU_FINALIZE 148
#define INTEL_SIP_SMC_FCS_MAC_VERIFY_SMMU_FINALIZE \
	INTEL_SIP_SMC_STD_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_MAC_VERIFY_SMMU_FINALIZE)

/**
 * Request INTEL_SIP_SMC_FCS_ECDSA_SHA2_DATA_SIGNING_SMMU_UPDATE
 * Sync call to digital signature signing request on a data blob
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_ECDSA_SHA2_DATA_SIGNING_SMMU_UPDATE
 * a1 session ID
 * a2 context ID
 * a3 physical address of source
 * a4 size of source
 * a5 physical address of destination
 * a6 size of destination
 * a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_NOT_SUPPORTED or
 *    INTEL_SIP_SMC_STATUS_ERROR
 * a1 mailbox errors if a0 is INTEL_SIP_SMC_STATUS_ERROR
 * a2 physical address of response data
 * a3 size of response data
 */
#define INTEL_SIP_SMC_FUNCID_ECDSA_SHA2_DATA_SIGNING_SMMU_UPDATE 149
#define INTEL_SIP_SMC_FCS_ECDSA_SHA2_DATA_SIGNING_SMMU_UPDATE \
	INTEL_SIP_SMC_STD_CALL_VAL(INTEL_SIP_SMC_FUNCID_ECDSA_SHA2_DATA_SIGNING_SMMU_UPDATE)

/**
 * Request INTEL_SIP_SMC_FCS_ECDSA_SHA2_DATA_SIGNING_SMMU_FINALIZE
 * Sync call to digital signature signing request on a data blob
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_ECDSA_SHA2_DATA_SIGNING_SMMU_FINALIZE
 * a1 session ID
 * a2 context ID
 * a3 physical address of source
 * a4 size of source
 * a5 physical address of destation
 * a6 size of destation
 * a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_NOT_SUPPORTED or
 *    INTEL_SIP_SMC_STATUS_ERROR
 * a1 mailbox errors if a0 is INTEL_SIP_SMC_STATUS_ERROR
 * a2 physical address of response data
 * a3 size of response data
 */
#define INTEL_SIP_SMC_FUNCID_ECDSA_SHA2_DATA_SIGNING_SMMU_FINALIZE 150
#define INTEL_SIP_SMC_FCS_ECDSA_SHA2_DATA_SIGNING_SMMU_FINALIZE \
	INTEL_SIP_SMC_STD_CALL_VAL(INTEL_SIP_SMC_FUNCID_ECDSA_SHA2_DATA_SIGNING_SMMU_FINALIZE)

/**
 * Request INTEL_SIP_SMC_FCS_ECDSA_SHA2_DATA_SIGNATURE_VERIFY_SMMU_UPDATE
 * Sync call to send digital signature verify request
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_ECDSA_SHA2_DATA_SIGNATURE_VERIFY_SMMU_UPDATE
 * a1 session ID
 * a2 context ID
 * a3 physical address of source (contain user data)
 * a4 size of source
 * a5 physical address of destination
 * a6 size of destination
 * a7 size of user data
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_NOT_SUPPORTED or
 *    INTEL_SIP_SMC_STATUS_ERROR
 * a1 mailbox errors if a0 is INTEL_SIP_SMC_STATUS_ERROR
 * a2 physical address of response data
 * a3 size of response data
 */
#define INTEL_SIP_SMC_FUNCID_FCS_ECDSA_SHA2_DATA_SIGNATURE_VERIFY_SMMU_UPDATE 151
#define INTEL_SIP_SMC_FCS_ECDSA_SHA2_DATA_SIGNATURE_VERIFY_SMMU_UPDATE \
	INTEL_SIP_SMC_STD_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_ECDSA_SHA2_DATA_SIGNATURE_VERIFY_SMMU_UPDATE)

/**
 * Request INTEL_SIP_SMC_FCS_ECDSA_SHA2_DATA_SIGNATURE_VERIFY_SMMU_FINALIZE
 * Sync call to send digital signature verify request
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FCS_ECDSA_SHA2_DATA_SIGNATURE_VERIFY_SMMU_FINALIZE
 * a1 session ID
 * a2 context ID
 * a3 physical address of source
 * a4 size of source
 * a5 physical address of destation
 * a6 size of destation
 * a7 size of user data
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_NOT_SUPPORTED or
 *    INTEL_SIP_SMC_STATUS_ERROR
 * a1 mailbox errors if a0 is INTEL_SIP_SMC_STATUS_ERROR
 * a2 physical address of response data
 * a3 size of response data
 */
#define INTEL_SIP_SMC_FUNCID_FCS_ECDSA_SHA2_DATA_SIGNATURE_VERIFY_SMMU_FINALIZE 152
#define INTEL_SIP_SMC_FCS_ECDSA_SHA2_DATA_SIGNATURE_VERIFY_SMMU_FINALIZE \
	INTEL_SIP_SMC_STD_CALL_VAL(INTEL_SIP_SMC_FUNCID_FCS_ECDSA_SHA2_DATA_SIGNATURE_VERIFY_SMMU_FINALIZE)

/**
 * Request INTEL_SIP_SMC_SEU_ERR_STATUS
 * Sync call to get previous Double Bit ECC error information.
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_SEU_ERR_STATUS
 * a1-7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_NOT_SUPPORTED or
 *    INTEL_SIP_SMC_STATUS_ERROR
 * a1 error count of response data
 * a2 sector address of response data
 * a3 error information
 */
#define INTEL_SIP_SMC_FUNCID_SEU_ERR_STATUS 153
#define INTEL_SIP_SMC_SEU_ERR_STATUS \
		INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_SEU_ERR_STATUS)

/**
 * Request INTEL_SIP_SMC_SAFE_INJECT_SEU_ERR
 * Sync call to inject SEU Error.
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_FUNCID_SAFE_INJECT_SEU_ERR
 * a1 Number of words
 * a2-7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK, INTEL_SIP_SMC_STATUS_NOT_SUPPORTED or
 *    INTEL_SIP_SMC_STATUS_ERROR
 * a1-a3 Not used
 */
#define INTEL_SIP_SMC_FUNCID_SAFE_INJECT_SEU_ERR 154
#define INTEL_SIP_SMC_SAFE_INJECT_SEU_ERR \
		INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_FUNCID_SAFE_INJECT_SEU_ERR)

/**
 * Request INTEL_SIP_SMC_SVC_VERSION
 *
 * Sync call used to query the SIP SMC API Version
 *
 * Call register usage:
 * a0 INTEL_SIP_SMC_SVC_VERSION
 * a1-a7 not used
 *
 * Return status:
 * a0 INTEL_SIP_SMC_STATUS_OK
 * a1 Major
 * a2 Minor
 */
#define INTEL_SIP_SMC_SVC_FUNCID_VERSION 512
#define INTEL_SIP_SMC_SVC_VERSION \
		INTEL_SIP_SMC_FAST_CALL_VAL(INTEL_SIP_SMC_SVC_FUNCID_VERSION)
#endif
