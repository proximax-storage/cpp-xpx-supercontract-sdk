use std::io::{BufReader, Read};

pub mod blockchain;
pub mod dir_iterator;
pub mod file;
pub mod filesystem;
pub mod internet;

// extern "C" {
//     fn print(s: u32);
// }

#[no_mangle]
pub unsafe extern "C" fn run() -> u32 {
    let internet = internet::Internet::new("example.com", true).unwrap();
    let mut reader = BufReader::with_capacity(16 * 1024, internet);
    // let identifier = 123456789i64;
    let mut buf = Vec::new();
    // buf.resize(20 * 1048576, 0);
    let res = reader.read_to_end(&mut buf).unwrap();
    // print(res as u32);
    if res == 0 {
        return 0;
    }
    let expected = "9CZBFJ2y4gG7MuLOkvXzz8EeGeO2q9gUmhoQLXaNIUaubMTHF2EYhihKiILfHew6bdUVKdJoKxsn7DBguI5iGrUHC7xu6WEfXLVWMCn8vYNmj1NQjpA7C1khqL4vU76VZ3yqNMamQLafc7Trc04oPdab4dPiS7f9m10UhnTG1EuEF4pr5PjyQU6hOoswE1Ey5VVu1ApH3AnQzHuJeWe15SpgaNQzminHP4lo4gwXeBCGRnWr6G7DqTxZv6CfkNcQEGM7";
    let actual = std::str::from_utf8_unchecked(&buf[0..res as usize]);
    assert_eq!(actual, expected);
    return 1;
}
