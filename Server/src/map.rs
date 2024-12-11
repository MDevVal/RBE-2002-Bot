use anyhow::{Context, Ok};
use pathfinding::{grid::Grid, prelude::dijkstra};

#[derive(Clone, Copy)]
enum Cell {
    Empty,
    Romi,
    Obstacle,
    Garbage,
}

const WIDTH: usize = 32;
const HEIGHT: usize = 32;
pub struct Map {
    map: [[Cell; HEIGHT]; WIDTH],
    grid: Grid,
}

impl Map {
    pub fn new() -> Self {
        let map = [[Cell::Empty; HEIGHT]; WIDTH];

        let mut grid = Grid::new(WIDTH, HEIGHT);
        grid.fill();
        grid.remove_vertex((0,2));
        grid.remove_vertex((1,3));
        grid.remove_vertex((2,4));
        Self { map, grid }
    }

    pub fn route(
        &self,
        from: (usize, usize),
        to: (usize, usize),
    ) -> anyhow::Result<Vec<(usize, usize)>> {
        let route = dijkstra(
            &from,
            |n| self.grid.neighbours(n.clone()).into_iter().map(|n| (n, 1)), |n| *n == to,
        )
        .context("no route")?;

        Ok(route.0)
    }
}
