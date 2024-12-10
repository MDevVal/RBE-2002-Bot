use std::{sync::Arc, time::Duration};

use axum::{body::Bytes, extract::{Path, State}};
use dashmap::DashMap;
use protobuf::{EnumOrUnknown, Message, MessageField, SpecialFields};
use tokio::{sync::{mpsc, oneshot}, time::timeout};
use tracing::{error, info};
use anyhow::{Context, Ok, Result};
use tracing::trace;

use crate::{protos::message::{server_command::{self}, GridCell, RomiData, ServerCommand}, ServerState};

pub type RomiStore = DashMap<u8, Romi>;

type Callback = oneshot::Sender<RomiData>;

pub type RomiCommander = mpsc::Sender<(ServerCommand, Callback)>;

pub trait Robot {
    async fn go_cell(&self, x: i32, y: i32) -> Result<()>;
}

impl Robot for RomiCommander {
    async fn go_cell(&self, x: i32, y: i32) -> Result<()> {
        let mut command = ServerCommand::new();
        command.targetGridCell =  MessageField::some(GridCell { x, y, special_fields: SpecialFields::new() });
        let dat = execute(self, command).await?;
        info!("recv: {dat:?}");

        Ok(())
    }
}

pub struct Romi {
    commands: mpsc::Receiver<(ServerCommand, Callback)>,
    callback: Option<Callback>,
    position: GridCell,
}

pub async fn next_state(
    Path(id): Path<u8>,
    state: State<Arc<ServerState>>,
    data: Bytes) -> Vec<u8> {

    trace!("state request from {id}");

    match update_state(state, id, data).await {
        Result::Ok(command) => {
            command
        }
        Err(e) => {
            error!("update state fail: {e}");
            let mut command = ServerCommand::new();
            command.state = Some(EnumOrUnknown::new(server_command::State::IDLE));
        
            command
        }
    }.write_to_bytes().unwrap()
}

async fn update_state(
    state: State<Arc<ServerState>>,
    id: u8,
    data: Bytes) -> Result<ServerCommand> {
    let romidata = RomiData::parse_from_bytes(&data)?;

    let mut romi = match state.romis.get_mut(&id) {
        Some(x) => x,
        None => {
            let (commander, commands) = mpsc::channel(1);
            state.romis.insert(id, Romi {
                commands,
                callback: None,
                position: GridCell::new(),
            });
            state.commanders.send(commander).await?;
            state.romis.get_mut(&id).unwrap()
        }
    };

    romi.position = *romidata.gridLocation.clone().0.context("request missing position")?;

    if let Some(callback)  = romi.callback.take() {
        callback.send(romidata).ok().context("callback dead")?;
    }

    // fun solution to an invented problem
    let (command, callback) = timeout(
        Duration::from_millis(500), 
        romi.commands.recv()).await
            .context("command channel closed")?
            .context("command timeout")?;

    romi.callback = Some(callback);

    Ok(command)
}

pub async fn execute(romi: &RomiCommander, command: ServerCommand) -> Result<RomiData> {
    let (tx, rx) = oneshot::channel();
    romi.send((command, tx)).await?;
    Ok(rx.await?)
}
