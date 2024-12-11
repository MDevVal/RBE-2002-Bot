use anyhow::{Context, Ok};
use pathfinding::{grid::Grid, prelude::{astar, dijkstra}};

#[derive(Clone, Copy)]
enum Cell {
    Empty,
    Romi,
    Obstacle, 
    Garbage, 
}

const WIDTH: usize = 24;
const HEIGHT: usize = 8;
pub struct Map {
    map: [[Cell; HEIGHT]; WIDTH],
    grid: Grid,
}

impl Map {
    pub fn new() -> Self {
        let map = [[Cell::Empty;HEIGHT];WIDTH];

        let mut grid = Grid::new(WIDTH, HEIGHT);
        grid.fill();
        Self { map, grid }
    }

    pub fn route(&self, from: (usize, usize), to: (usize, usize)) -> anyhow::Result<(usize, usize)>{
        let route = dijkstra(
            &from,
            |n| self.grid.neighbours(n.clone()).into_iter().map(|n| (n,1)),
            |n| *n == to,
        ).context("no route")?;

        Ok(route.0.get(0).context("no route")?.clone())
    }
}
