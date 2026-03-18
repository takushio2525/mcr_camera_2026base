/*---------------------------------------------------------------------------/
/  FatFs - FAT file system module configuration file  R0.11a (C)ChaN, 2015
/  mcr_camera_2026base 用カスタム設定
/---------------------------------------------------------------------------*/

#define _FFCONF 64180	/* Revision ID */

#define FFS_DBG			0

/*---------------------------------------------------------------------------/
/ Function Configurations
/---------------------------------------------------------------------------*/

#define _FS_READONLY	0
/* 読み書き両対応 */

#define _FS_MINIMIZE	0
/* 全API有効 */

#define	_USE_STRFUNC	1
/* f_printf等の文字列関数を有効化（CSV書き出しに使用） */

#define _USE_FIND		0

#define	_USE_MKFS		0
/* フォーマット機能は不要（PCでフォーマット済みのSDカードを使う） */

#define	_USE_FASTSEEK	0

#define _USE_LABEL		0

#define	_USE_FORWARD	0

/*---------------------------------------------------------------------------/
/ Locale and Namespace Configurations
/---------------------------------------------------------------------------*/

#define _CODE_PAGE	437
/* ASCII のみ使用 */

#define	_USE_LFN	0
#define	_MAX_LFN	255
/* LFN（長いファイル名）は不要。8.3形式で十分 */

#define	_LFN_UNICODE	0

#define _STRF_ENCODE	3

#define _FS_RPATH	0

/*---------------------------------------------------------------------------/
/ Drive/Volume Configurations
/---------------------------------------------------------------------------*/

#define _VOLUMES	1
/* ドライブ1つ（SDカードのみ） */

#define _STR_VOLUME_ID	0
#define _VOLUME_STRS	"SD"

#define	_MULTI_PARTITION	0

#define	_MIN_SS		512
#define	_MAX_SS		512
/* セクタサイズ固定512バイト */

#define	_USE_TRIM	0

#define _FS_NOFSINFO	0

/*---------------------------------------------------------------------------/
/ System Configurations
/---------------------------------------------------------------------------*/

#define	_FS_TINY	0

#define _FS_NORTC	1
#define _NORTC_MON	3
#define _NORTC_MDAY	18
#define _NORTC_YEAR	2026
/* RTCなし: 固定タイムスタンプを使用 */

#define	_FS_LOCK	0

#define _FS_REENTRANT	0
#define _FS_TIMEOUT		1000
#define	_SYNC_t			int
/* シングルスレッド（ベアメタル）のため再入制御不要 */

#define _WORD_ACCESS	0
/* Cortex-A9 はアラインメントに注意が必要なためバイトアクセス */

#define FLUSH_ON_NEW_CLUSTER    0
#define FLUSH_ON_NEW_SECTOR     1
/* セクタ単位でフラッシュ（データ損失リスク低減） */
