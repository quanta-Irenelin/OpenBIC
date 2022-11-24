/*
** Include Files
*/
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "srmburn.h"
#include "plat_i2c.h"
#include <drivers/disk.h>


uint8_t fw_update(uint32_t offset, uint16_t msg_len, uint8_t *msg_buf, bool sector_end,
		  uint8_t flash_position)
{
	static bool is_init = 0;
	static uint8_t *txbuf = NULL;
	static uint32_t buf_offset = 0;
	uint32_t ret = 0;
	const struct device *flash_dev;

	if (!is_init) {
		SAFE_FREE(txbuf);
		txbuf = (uint8_t *)malloc(32);
		if (txbuf == NULL) { // Retry alloc
			k_msleep(100);
			txbuf = (uint8_t *)malloc(32);
		}
		if (txbuf == NULL) {
			LOG_ERR("i2c index %d, failed to allocate txbuf.", flash_position);
			return FWUPDATE_OUT_OF_HEAP;
		}
		is_init = 1;
		buf_offset = 0;
		k_msleep(10);
	}

	if ((buf_offset + msg_len) > 32) {
		LOG_ERR("i2c index %d, recv data 0x%x over sector size 0x%x", flash_position,
			buf_offset + msg_len, SECTOR_SZ_64K);
		SAFE_FREE(txbuf);
		k_msleep(10);
		is_init = 0;
		return FWUPDATE_OVER_LENGTH;
	}

	if ((offset % 32) != buf_offset) {
		LOG_ERR("i2c index %d, recorded offset 0x%x but updating 0x%x\n", flash_position,
			buf_offset, offset % SECTOR_SZ_64K);
		SAFE_FREE(txbuf);
		txbuf = NULL;
		k_msleep(10);
		is_init = 0;
		return FWUPDATE_REPEATED_UPDATED;
	}

	LOG_DBG("i2c index %d, update offset 0x%x 0x%x, msg_len %d, sector_end %d,"
		" msg_buf: 0x%2x 0x%2x 0x%2x 0x%2x\n",
		flash_position, offset, buf_offset, msg_len, sector_end, msg_buf[0], msg_buf[1],
		msg_buf[2], msg_buf[3]);

	memcpy(&txbuf[buf_offset], msg_buf, msg_len);
	buf_offset += msg_len;

	// Update fmc while collect 64k bytes data or BMC signal last image package with target | 0x80
	if ((buf_offset == SECTOR_SZ_64K) || sector_end) {
		flash_dev = device_get_binding(flash_device_list[flash_position].name);
		if (!flash_device_list[flash_position].isinit) {
			uint8_t rc = 0;
			rc = spi_nor_re_init(flash_dev);
			if (rc != 0) {
				return rc;
			}
			flash_device_list[flash_position].isinit = true;
		}
		uint8_t sector = 16;
		uint32_t txbuf_offset;
		uint32_t update_offset;

		for (int i = 0; i < sector; i++) {
			txbuf_offset = SECTOR_SZ_4K * i;
			update_offset = (offset / SECTOR_SZ_64K) * SECTOR_SZ_64K + txbuf_offset;
			ret = do_update(flash_dev, update_offset, &txbuf[txbuf_offset],
					SECTOR_SZ_4K);
			if (ret) {
				LOG_ERR("Failed to update SPI, status %d", ret);
				break;
			}
		}
		if (!ret) {
			LOG_INF("Update success");
		}
		SAFE_FREE(txbuf);
		k_msleep(10);
		is_init = 0;

		LOG_DBG("Update 0x%x, offset 0x%x, SECTOR_SZ_16K 0x%x\n",
			(offset / SECTOR_SZ_16K) * SECTOR_SZ_16K, offset, SECTOR_SZ_16K);

		return ret;
	}

	return FWUPDATE_SUCCESS;
}