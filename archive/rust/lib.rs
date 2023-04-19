#![feature(lang_items)]
#![no_std]

#[lang = "eh_personality"]
extern fn eh_personality() {
}

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    loop {}
}



#[no_mangle]
pub extern fn kmain() -> ! {
    // The VGA buffer starts at physical address 0xb8000.
    let vga_buffer = 0xb8000 as *mut u16;

    // Write some characters to the buffer.
    for i in 0..25 {
        for j in 0..80 {
            let offset = i * 80 + j;
            let color = 0x0F00; // White on black
            let character = b'A';
            vga_buffer.offset(offset as isize).write_volatile(color | (character as u16));
        }
    }

    loop { }
}
