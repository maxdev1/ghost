ENTRY (loadKernel)

SECTIONS
{
	/* Start at this address*/
	. = 0xC0000000;
	
	/* Text section */
	textSectionStart = .;
	.text BLOCK(4K) : ALIGN(4K)
	{
		*(.text)
	}
	textSectionEnd = .;
	
	/* Read-only section */
	rodataSectionStart = .;
	.rodata BLOCK(4K) : ALIGN(4K)
	{
        startConstructors = .;
        *(SORT(.ctors*))
        endConstructors = .;
        
		*(.rodata)
	}
	rodataSectionEnd = .;
	
	/* Data section */
	dataSectionStart = .;
	.data BLOCK(4K) : ALIGN(4K)
	{
		*(.data)
	}
	dataSectionEnd = .;
	
	/* BSS section */
	bssSectionStart = .;
	.bss BLOCK(4K) : ALIGN(4K)
	{
		*(COMMON)
		*(.bss)
	}
	bssSectionEnd = .;
	   
	/* Align the end of the kernel to 0x1000 */
	endKernel = ALIGN(4K);
}