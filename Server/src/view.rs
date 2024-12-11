use axum::{
    extract::{ws::{CloseFrame, Message, WebSocket, WebSocketUpgrade}, ConnectInfo, State},
    response::IntoResponse,
    routing::any,
    Router,
};
use axum_extra::TypedHeader;
use serde::{Deserialize, Serialize};
use std::{borrow::Cow, net::SocketAddr, ops::ControlFlow, path::PathBuf, sync::Arc};
use futures_util::StreamExt;
use futures_util::SinkExt;

use crate::ServerState;

#[derive(Serialize, Deserialize)]
struct RomiDatum {
    x: i32,
    y: i32,
    name: u8
}

pub async fn ws_handler(
    ws: WebSocketUpgrade,
    ConnectInfo(addr): ConnectInfo<SocketAddr>,
    state: State<Arc<ServerState>>,
) -> impl IntoResponse {
    ws.on_upgrade(move |socket| handle_socket(socket, addr, state))
}

/// Actual websocket statemachine (one will be spawned per connection)
async fn handle_socket(mut socket: WebSocket, who: SocketAddr,
    state: State<Arc<ServerState>>,
    ) {
    let (mut sender, mut receiver) = socket.split();

    let mut send_task = tokio::spawn(async move {
        loop {
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
            if sender
                .send(Message::Text(format!("Server message ...")))
                .await
                .is_err()
            {
                break;
            }

            tokio::time::sleep(std::time::Duration::from_millis(300)).await;
        }

        println!("Sending close to {who}...");
        if let Err(e) = sender
            .send(Message::Close(Some(CloseFrame {
                code: axum::extract::ws::close_code::NORMAL,
                reason: Cow::from("Goodbye"),
            })))
            .await
        {
            println!("Could not send Close due to {e}, probably it is ok?");
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
