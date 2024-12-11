use anyhow::{Context, Ok};
use pathfinding::{grid::Grid, prelude::dijkstra};

#[derive(Clone, Copy, PartialEq, Eq)]
pub enum Cell {
    Empty,
    Romi,
    Obstacle,
    Garbage,
}

const WIDTH: usize = 32;
const HEIGHT: usize = 32;
pub struct Map {
    map: [[Cell; WIDTH]; HEIGHT],
    grid: Grid,
}

impl Map {
    pub fn new() -> Self {
        let map = [[Cell::Empty; HEIGHT]; WIDTH];
        let mut grid = Grid::new(WIDTH, HEIGHT);
        grid.fill();
        Self { map, grid }
    }

    pub fn get_all(&self, variety: Cell) -> Vec<(i32, i32)> {
        let mut matches = Vec::new();
        for (i, row) in self.map.iter().enumerate() {
            for (j, cell) in row.iter().enumerate() {
                if *cell == variety {
                    matches.push((i as i32, j as i32));
                }
            }
        }
        matches
    }

    pub fn insert_obstacle(&mut self, pos: (usize,usize)) {
        self.grid.remove_vertex(pos);
        self.map[pos.0][pos.1] = Cell::Obstacle;
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
