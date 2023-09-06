pub mod blockchain;
pub mod dir_iterator;
pub mod file;
pub mod filesystem;
pub mod internet;

use blockchain::get_call_params;

#[no_mangle]
pub unsafe extern "C" fn run() -> u32 {
    let binding = get_call_params();
    let params = binding.get(0).unwrap();
    // assert_eq!(params.get(0), Some(&65u8));
    // assert_eq!(params.get(0), Some(&66u8));

    return *params as u32;
}