use std::{sync::Arc, time::Duration};

use anyhow::{Context, Ok, Result};
use axum::{
    body::Bytes,
    extract::{Path, State},
};
use dashmap::DashMap;
use protobuf::{EnumOrUnknown, Message, MessageField, SpecialFields};
use tokio::{
    sync::{mpsc, oneshot, RwLock},
    time::{sleep, timeout},
};
use tracing::trace;
use tracing::{error, info};

use crate::{
    map::Map,
    protos::message::{
        server_command::{self},
        GridCell, RomiData, ServerCommand,
    },
    ServerState,
};

pub type RomiStore = DashMap<u8, Romi>;

type Callback = oneshot::Sender<RomiData>;

pub struct RomiCommander {
    sender: mpsc::Sender<(ServerCommand, Callback)>,
    position: GridCell,
}

impl RomiCommander {
    pub async fn go_cell(&mut self, x: i32, y: i32) -> Result<RomiData> {
        let mut command = ServerCommand::new();
        command.targetGridCell = MessageField::some(GridCell {
            x,
            y,
            special_fields: SpecialFields::new(),
        });
        command.state = Some(EnumOrUnknown::new(server_command::State::LINING));
        command.baseSpeed = 10.;
        let dat = self.execute(command).await?;
        info!("recv: {dat:?}");

        Ok(dat)
    }

    pub async fn execute(&mut self, command: ServerCommand) -> Result<RomiData> {
        let (tx, rx) = oneshot::channel();
        self.sender.send((command, tx)).await?;

        let romi_data = rx.await?;
        self.position = *romi_data
            .gridLocation
            .0
            .clone()
            .context("request missing position")?;
        Ok(romi_data)
    }

    pub fn get_pos(&self) -> (i32, i32) {
        (self.position.x, self.position.y)
    }

    pub async fn route(
        &mut self,
        map: &RwLock<Map>,
        position: (usize, usize),
    ) -> anyhow::Result<()> {
        loop {
            let get_pos = self.get_pos();
            let route = map.read().await.route(
                (get_pos.0 as usize, get_pos.1 as usize),
                (position.0, position.1),
            )?;
            if route.len() > 1 {
                self.go_cell(route[1].0 as i32, route[1].1 as i32).await?;
            } else {
                break;
            }
        }

        Ok(())
    }
}

pub struct Romi {
    commands: mpsc::Receiver<(ServerCommand, Callback)>,
    callback: Option<Callback>,
    position: GridCell,
}

impl Romi {
    pub fn position(&self) -> &GridCell {
        &self.position
    }
}

pub async fn next_state(
    Path(id): Path<u8>,
    state: State<Arc<ServerState>>,
    data: Bytes,
) -> Vec<u8> {
    trace!("state request from {id}");

    if data.len() == 0 {
        trace!("empty");
        return ServerCommand::new().write_to_bytes().unwrap()
    }

    match update_state(state, id, data).await {
        Result::Ok(command) => {
            trace!("sending {command:?}");
            command
        },
        Err(e) => {
            error!("update state fail: {e}");
            let mut command = ServerCommand::new();
            //command.state = Some(EnumOrUnknown::new(server_command::State::IDLE));

            command
        }
    }
    .write_to_bytes()
    .unwrap()
}

async fn update_state(
    state: State<Arc<ServerState>>,
    id: u8,
    data: Bytes,
) -> Result<ServerCommand> {
    let romidata = RomiData::parse_from_bytes(&data)?;
    trace!("recv: {romidata:?}");
    let grid_cell = *romidata
        .gridLocation
        .clone()
        .0
        .context("request missing position")?;

    let mut romi = match state.romis.get_mut(&id) {
        Some(x) => x,
        None => {
            let (commander, commands) = mpsc::channel(1);
            state.romis.insert(
                id,
                Romi {
                    commands,
                    callback: None,
                    position: GridCell::new(),
                },
            );
            state
                .commanders
                .send(RomiCommander {
                    sender: commander,
                    position: grid_cell.clone(),
                })
                .await?;
            state.romis.get_mut(&id).unwrap()
        }
    };

    romi.position = grid_cell;

    if let Some(callback) = romi.callback.take() {
        callback.send(romidata).ok().context("callback dead")?;
    }

    // fun solution to an invented problem
    let (command, callback) = timeout(Duration::from_millis(500), romi.commands.recv())
        .await
        .context("command channel closed")?
        .context("command timeout")?;

    romi.callback = Some(callback);

    Ok(command)
}
