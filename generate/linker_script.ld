/* SPIフラッシュ起動対応リンカスクリプト (DAPLink MSD書き込み用) */
/* 元のRAM用リンカスクリプトは linker_script_ram.ld に保存 */

MEMORY {
    BOOT_LOADER : ORIGIN = 0x18000000, LENGTH = 0x00004000
    SFLASH      : ORIGIN = 0x18004000, LENGTH = 0x07FFC000
    RAM         : ORIGIN = 0x20020000, LENGTH = 0x00500000
}

SECTIONS
{
    /* ブートローダー領域 (0x18000000 - 0x18003FFF) */
    /* SPIフラッシュの初期化とベクタテーブルへのジャンプを行う */
    .boot :
    {
        KEEP(*(.boot_loader))
    } > BOOT_LOADER

    /* ベクタテーブル (SFLASHの先頭 0x18004000) */
    /* ブートローダーがここにジャンプする */
    .fvectors :
    {
        KEEP(*(.fvectors))
    } > SFLASH

    /* テキスト領域 (SFLASH上で実行) */
    .text :
    {
        *(.text)
        *(.text.*)
    } > SFLASH

    /* 割り込みベクタ */
    .rvectors :
    {
        _rvectors_start = .;
        KEEP(*(.rvectors))
        _rvectors_end = .;
    } > SFLASH

    .init :
    {
        *(.init)
    } > SFLASH

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
        PROVIDE_HIDDEN (__exidx_start = .);
    } > SFLASH

    .ARM.exidx :
    {
        *(.ARM.exidx .gnu.linkonce.armexidx.*)
        PROVIDE_HIDDEN (__exidx_end = .);
    } > SFLASH

    .fini :
    {
        *(.fini)
    } > SFLASH

    .got :
    {
        *(.got)
        *(.got.plt)
    } > SFLASH

    /* 読み取り専用データ (SFLASH上) */
    .rodata :
    {
        *(.rodata)
        *(.rodata.*)
        _erodata = .;
    } > SFLASH

    .eh_frame_hdr :
    {
        *(.eh_frame_hdr)
    } > SFLASH

    .eh_frame :
    {
        *(.eh_frame)
    } > SFLASH

    .jcr :
    {
        *(.jcr)
    } > SFLASH

    /* コンストラクタ/デストラクタ */
    .tors :
    {
        __CTOR_LIST__ = .;
        . = ALIGN(2);
        __ctors = .;
        *(.ctors)
        __ctors_end = .;
        __CTOR_END__ = .;
        __DTOR_LIST__ = .;
        ___dtors = .;
        *(.dtors)
        ___dtors_end = .;
        __DTOR_END__ = .;
        . = ALIGN(2);
        _mdata = .;
    } > SFLASH

    /* 初期化済みデータ (SFLASHからRAMへコピーされる) */
    .data 0x20020000 : AT (_mdata)
    {
        _data = .;
        *(.data)
        *(.data.*)
        _edata = .;
    } > RAM

    /* 未初期化データ (RAM上、ゼロクリアされる) */
    .bss :
    {
        PROVIDE(__bss_start__ = .);
        _bss = .;
        *(.bss)
        *(.bss.**)
        *(COMMON)
        PROVIDE(__bss_end__ = .);
        _ebss = .;
        _end = .;
        PROVIDE(end = .);
    } > RAM

    /* スタック */
    .stack 0x20900000 (NOLOAD)  : AT (0x20900000)
    {
        _stack = .;
    } > RAM
}
