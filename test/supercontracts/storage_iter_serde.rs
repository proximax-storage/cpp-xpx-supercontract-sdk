pub mod blockchain;
pub mod dir_iterator;
pub mod file;
pub mod filesystem;
pub mod internet;

use serde_json::Value;
use std::{io::{Read, Write}, collections::HashMap};

use crate::{filesystem::{create_dir, move_filesystem_entry}, dir_iterator::DirIterator};

#[no_mangle]
pub extern "C" fn run() -> i32 {
    // create dir
    create_dir("unsorted").unwrap();
    create_dir("over").unwrap();
    create_dir("under").unwrap();

    // read a few coin price
    let mut btc_buffer = Vec::new();
    {
        let mut internet = internet::Internet::new("https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=usd",true,).unwrap();
        let len = internet.read_to_end(&mut btc_buffer).unwrap();
        assert_eq!(len, 25);

        let mut file = file::FileWriter::new("unsorted/btc.txt").unwrap();
        let res = file.write(&btc_buffer).unwrap();
        assert_eq!(res, 25);
        file.flush().unwrap();
    }

    let mut eth_buffer = Vec::new();
    {
        let mut internet = internet::Internet::new("https://api.coingecko.com/api/v3/simple/price?ids=ethereum&vs_currencies=usd&precision=0", true).unwrap();
        let len = internet.read_to_end(&mut eth_buffer).unwrap();
        assert_eq!(len, 25);

        let mut file = file::FileWriter::new("unsorted/eth.txt").unwrap();
        let res = file.write(&eth_buffer).unwrap();
        assert_eq!(res, 25);
        file.flush().unwrap();
    }

    let mut bnb_buffer = Vec::new();
    {
        let mut internet = internet::Internet::new("https://api.coingecko.com/api/v3/simple/price?ids=binancecoin&vs_currencies=usd&precision=0", true).unwrap();
        let len = internet.read_to_end(&mut bnb_buffer).unwrap();
        assert_eq!(len, 27);

        let mut file = file::FileWriter::new("unsorted/bnb.txt").unwrap();
        let res = file.write(&bnb_buffer).unwrap();
        assert_eq!(res, 27);
        file.flush().unwrap();
    }

    // serialize the data
    let mut map: HashMap<String, i64> = HashMap::new();
    let parsed_btc: Value = serde_json::from_str(&String::from_utf8(btc_buffer).unwrap()).unwrap();
    let btc_value = parsed_btc["bitcoin"]["usd"].as_i64().unwrap();
    map.insert("btc".to_string(), btc_value);
    let parsed_eth: Value = serde_json::from_str(&String::from_utf8(eth_buffer).unwrap()).unwrap();
    let eth_value = parsed_eth["ethereum"]["usd"].as_i64().unwrap();
    map.insert("eth".to_string(), eth_value);
    let parsed_bnb: Value = serde_json::from_str(&String::from_utf8(bnb_buffer).unwrap()).unwrap();
    let bnb_value = parsed_bnb["binancecoin"]["usd"].as_i64().unwrap();
    map.insert("bnb".to_string(), bnb_value);
    assert_eq!(map.len(), 3);

    if blockchain::get_block_height() % 240 == 0 {
        for (name, number) in &map {
            let old_path = format!("unsorted/{}.txt", name);
            if number > &5000 {
                let new_path = format!("over/{}.txt", name);
                move_filesystem_entry(&old_path, &new_path).unwrap();
            }else{
                let new_path = format!("under/{}.txt", name);
                move_filesystem_entry(&old_path, &new_path).unwrap();
            }
        }
    }

    let mut res = 1i32;
    {
        let mut iterator = DirIterator::new("over", false);

        let expected = [
            "btc.txt",
        ];

        while let Some(entry) = iterator.next() {
            let path = &entry.name;
            if !expected.contains(&path.as_str()) {
                res = 99;
            }
        }
    }
    assert_eq!(res, 1);

    {
        let mut iterator = DirIterator::new("under", false);

        let expected = [
            "eth.txt",
            "bnb.txt"
        ];

        while let Some(entry) = iterator.next() {
            let path = &entry.name;
            if !expected.contains(&path.as_str()) {
                res = 99;
            }
        }
    }
    assert_eq!(res, 1);

    return 1;
}
