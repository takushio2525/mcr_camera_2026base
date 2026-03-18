/*
 * diskio.cpp
 *
 *  FatFs ディスクI/Oブリッジ
 *  SDCard ドライバと FatFs コアを接続する
 */

#include "diskio.h"
#include "ff.h"
#include "../SDCard.h"

// ドライブ番号 (0 = SDカード)
#define SD_DRIVE 0

static DSTATUS sdStatus = STA_NOINIT;

extern "C" {

/*-----------------------------------------------------------------------*/
/* ディスクI/O初期化                                                      */
/*-----------------------------------------------------------------------*/
DSTATUS disk_initialize(BYTE pdrv)
{
  if (pdrv != SD_DRIVE) return STA_NOINIT;

  // SDCard はメインの init() で既に初期化済みの前提
  if (g_sdcard.isReady()) {
    sdStatus = 0;
  } else {
    sdStatus = STA_NOINIT;
  }

  return sdStatus;
}

/*-----------------------------------------------------------------------*/
/* ディスクステータス                                                     */
/*-----------------------------------------------------------------------*/
DSTATUS disk_status(BYTE pdrv)
{
  if (pdrv != SD_DRIVE) return STA_NOINIT;
  return sdStatus;
}

/*-----------------------------------------------------------------------*/
/* セクタ読み取り                                                         */
/*-----------------------------------------------------------------------*/
DRESULT disk_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count)
{
  if (pdrv != SD_DRIVE) return RES_PARERR;
  if (sdStatus & STA_NOINIT) return RES_NOTRDY;

  if (g_sdcard.readBlocks(buff, sector, count) == 0) {
    return RES_OK;
  }
  return RES_ERROR;
}

/*-----------------------------------------------------------------------*/
/* セクタ書き込み                                                         */
/*-----------------------------------------------------------------------*/
DRESULT disk_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count)
{
  if (pdrv != SD_DRIVE) return RES_PARERR;
  if (sdStatus & STA_NOINIT) return RES_NOTRDY;

  if (g_sdcard.writeBlocks(buff, sector, count) == 0) {
    return RES_OK;
  }
  return RES_ERROR;
}

/*-----------------------------------------------------------------------*/
/* ディスクI/O制御                                                        */
/*-----------------------------------------------------------------------*/
DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
  if (pdrv != SD_DRIVE) return RES_PARERR;
  if (sdStatus & STA_NOINIT) return RES_NOTRDY;

  switch (cmd) {
    case CTRL_SYNC:
      // 書き込みキャッシュのフラッシュ（SPIは即時反映なので何もしない）
      return RES_OK;

    case GET_SECTOR_COUNT:
      *(DWORD *)buff = g_sdcard.getSectorCount();
      return RES_OK;

    case GET_SECTOR_SIZE:
      *(WORD *)buff = 512;
      return RES_OK;

    case GET_BLOCK_SIZE:
      *(DWORD *)buff = 1;  // 消去ブロックサイズ（セクタ単位）
      return RES_OK;

    default:
      return RES_PARERR;
  }
}

} // extern "C"
