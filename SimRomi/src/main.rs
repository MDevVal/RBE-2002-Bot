use std::time::Duration;

use protobuf::{Message, MessageField, SpecialFields};
use protos::message::{GridCell, RomiData, ServerCommand};
use tokio::time;

mod protos;

#[tokio::main]
async fn main() {
    let romi_id: u8 = 42;
    let server_endpoint = format!("http://130.215.121.37:8080/nextState/{romi_id}");
    println!("Hello, world!");

    let mut position: (i32, i32) = (0,0);

    loop {
        let mut dat = RomiData::new();
        dat.gridLocation = MessageField::some(GridCell { x: position.0, y: position.1, special_fields: SpecialFields::new() });

        let response = reqwest::Client::new()
            .post(&server_endpoint)
            .body(dat.write_to_bytes().unwrap()).send().await.unwrap();

        let command = ServerCommand::parse_from_bytes(&response.bytes().await.unwrap()).unwrap();
        println!("received: {command:?}");

        let goal_position = (command.targetGridCell.x, command.targetGridCell.y);
        
        println!("going from {position:?} to {goal_position:?}");
        position = goal_position;

        time::sleep(Duration::from_millis(1000)).await;
    }
}
