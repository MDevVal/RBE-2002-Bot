use axum::{
    extract::{
        ws::{CloseFrame, Message, WebSocket, WebSocketUpgrade},
        ConnectInfo, State,
    },
    response::IntoResponse,
    routing::any,
    Router,
};
use axum_extra::TypedHeader;
use futures_util::SinkExt;
use futures_util::StreamExt;
use serde::{Deserialize, Serialize};
use std::{borrow::Cow, net::SocketAddr, ops::ControlFlow, path::PathBuf, sync::Arc};

use crate::ServerState;

#[derive(Serialize, Deserialize)]
struct RomiDatum {
    x: i32,
    y: i32,
    name: u8,
}

#[derive(Serialize, Deserialize)]
struct PosDatum {
    x: i32,
    y: i32,
}

#[derive(Serialize, Deserialize)]
enum Datum {
    Romi(RomiDatum),
    Garbage(PosDatum),
    Obstacle(PosDatum),
}

pub async fn ws_handler(
    ws: WebSocketUpgrade,
    ConnectInfo(addr): ConnectInfo<SocketAddr>,
    state: State<Arc<ServerState>>,
) -> impl IntoResponse {
    ws.on_upgrade(move |socket| handle_socket(socket, addr, state))
}

/// Actual websocket statemachine (one will be spawned per connection)
async fn handle_socket(mut socket: WebSocket, who: SocketAddr, state: State<Arc<ServerState>>) {
    let (mut sender, mut receiver) = socket.split();

    let mut send_task = tokio::spawn(async move {
        loop {
            let map = state.map.read().await;

            for (x,y) in map.get_all(crate::map::Cell::Obstacle) {
                sender.send(Message::Text(serde_json::to_string(&Datum::Obstacle(PosDatum {
                    x,
                    y,
                })).unwrap())).await.unwrap();
            }

            for (x,y) in map.get_all(crate::map::Cell::Garbage) {
                sender.send(Message::Text(serde_json::to_string(&Datum::Obstacle(PosDatum {
                    x,
                    y,
                })).unwrap())).await.unwrap();
            }

            sender.send(Message::Text(serde_json::to_string(&Datum::Romi(RomiDatum {
                x: 3,
                y: 5,
                name: 100
            })).unwrap())).await.unwrap();

            for romi in state.romis.iter() {
                let name = *romi.key();
                let pos = romi.value().position();
                let romi = RomiDatum {
                    x: pos.x,
                    y: pos.y,
                    name,
                };
                sender.send(Message::Text(serde_json::to_string(&romi).unwrap())).await.unwrap();
            }

            tokio::time::sleep(std::time::Duration::from_millis(300)).await;
        }
    });

    let mut recv_task = tokio::spawn(async move {
        let mut cnt = 0;
        while let Some(Ok(msg)) = receiver.next().await {
            cnt += 1;
            // print message and break if instructed to do so
            if process_message(msg, who).is_break() {
                break;
            }
        }
        cnt
    });

    // If any one of the tasks exit, abort the other.
    tokio::select! {
        _ = (&mut send_task) => {
            recv_task.abort();
        },
        _ = (&mut recv_task) => {
            send_task.abort();
        }
    }
}

/// helper to print contents of messages to stdout. Has special treatment for Close.
fn process_message(msg: Message, who: SocketAddr) -> ControlFlow<(), ()> {
    match msg {
        Message::Text(t) => {
            println!(">>> {who} sent str: {t:?}");
        }
        Message::Binary(d) => {
            println!(">>> {} sent {} bytes: {:?}", who, d.len(), d);
        }
        Message::Close(c) => {
            if let Some(cf) = c {
                println!(
                    ">>> {} sent close with code {} and reason `{}`",
                    who, cf.code, cf.reason
                );
            } else {
                println!(">>> {who} somehow sent close message without CloseFrame");
            }
            return ControlFlow::Break(());
        }

        Message::Pong(v) => {
            println!(">>> {who} sent pong with {v:?}");
        }
        // You should never need to manually handle Message::Ping, as axum's websocket library
        // will do so for you automagically by replying with Pong and copying the v according to
        // spec. But if you need the contents of the pings you can see them here.
        Message::Ping(v) => {
            println!(">>> {who} sent ping with {v:?}");
        }
    }
    ControlFlow::Continue(())
}
