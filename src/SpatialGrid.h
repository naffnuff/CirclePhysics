#pragma once

#include <vector>
#include <iostream>

#include "Vector2.h"

namespace CirclePhysics {

// A simple spatial grid for broad-phase collision detection of pairs of T
template <typename T>
class SpatialGrid
{
public:
    SpatialGrid(float worldBoundX, float worldBoundY, float cellSizeHint)
    {
        m_cellSize = std::max(cellSizeHint, 0.01f);
        updateDimensions(worldBoundX, worldBoundY);
    }

    // Update the grid dimensions if world bounds change
    void updateDimensions(float worldBoundX, float worldBoundY)
    {
        m_worldBoundX = worldBoundX;
        m_worldBoundY = worldBoundY;

        // Calculate the number of cells in each dimension
        int newCellCountX = (int)(2.0f * m_worldBoundX / m_cellSize) + 1;
        int newCellCountY = (int)(2.0f * m_worldBoundY / m_cellSize) + 1;

        if (newCellCountX != m_cellCountX || newCellCountY != m_cellCountY)
        {
            m_cellCountX = newCellCountX;
            m_cellCountY = newCellCountY;

            // Reserve space for all cells
            m_grid.resize(m_cellCountX * m_cellCountY);
        }
    }

    // Clear the grid for the next iteration
    void clear()
    {
        for (std::vector<T>& cell : m_grid)
        {
            cell.clear();
        }
    }

    // Insert a value into the grid
    void insert(T value, const Vector2& position, float radius)
    {
        int cellX, cellY;
        worldToCell(position, cellX, cellY);

        if (isValidCell(cellX, cellY))
        {
            m_grid[cellY * m_cellCountX + cellX].push_back(value);
        }
        else
        {
            // We can safely ignore circles that fall outside of the grid
            // as this will mainly happen at window resize and those circles
            // will quickly be brought back into the world
        }
    }

    // Get potential collision pairs
    void getPotentialCollisions(std::vector<std::pair<T, T>>& collisionPairs)
    {
        collisionPairs.clear();

        // For each cell in the grid...
        for (int y = 0; y < m_cellCountY; ++y)
        {
            for (int x = 0; x < m_cellCountX; ++x)
            {
                //...bring out the cache...
                const std::vector<T>& cell = m_grid[y * m_cellCountX + x];

                //...and first check for collisions within the same cell...
                for (size_t i = 0; i < cell.size(); ++i)
                {
                    //...with only those that come after
                    for (size_t j = i + 1; j < cell.size(); ++j)
                    {
                        collisionPairs.push_back(std::make_pair(cell[i], cell[j]));
                    }
                }

                // Then check the neighbouring cells, but only in right and down directions
                // (if we imagine this as a top-left to down-right search)
                
                // Right cell
                if (x + 1 < m_cellCountX)
                {
                    const std::vector<T>& rightCell = m_grid[y * m_cellCountX + (x + 1)];
                    for (T first : cell)
                    {
                        for (T second : rightCell)
                        {
                            collisionPairs.push_back(std::make_pair(first, second));
                        }
                    }
                }

                // Bottom cell
                if (y + 1 < m_cellCountY)
                {
                    const std::vector<T>& bottomCell = m_grid[(y + 1) * m_cellCountX + x];
                    for (T first : cell)
                    {
                        for (T second : bottomCell)
                        {
                            collisionPairs.push_back(std::make_pair(first, second));
                        }
                    }
                }

                // Bottom-right cell
                if (x + 1 < m_cellCountX && y + 1 < m_cellCountY)
                {
                    const std::vector<T>& bottomRightCell = m_grid[(y + 1) * m_cellCountX + (x + 1)];
                    for (T first : cell)
                    {
                        for (T second : bottomRightCell)
                        {
                            collisionPairs.push_back(std::make_pair(first, second));
                        }
                    }
                }

                // Bottom-left cell (if we are not at the left edge)
                if (x > 0 && y + 1 < m_cellCountY)
                {
                    const std::vector<T>& bottomLeftCell = m_grid[(y + 1) * m_cellCountX + (x - 1)];
                    for (T first : cell)
                    {
                        for (T second : bottomLeftCell)
                        {
                            collisionPairs.push_back(std::make_pair(first, second));
                        }
                    }
                }
            }
        }
    }

private:
    // Convert world coordinates to cell coordinates
    void worldToCell(const Vector2& position, int& cellX, int& cellY)
    {
        // Shift from world space (-boundX..boundX, -boundY..boundY) 
        // to grid space (0..2*boundX, 0..2*boundY)
        const float gridX = position.x + m_worldBoundX;
        const float gridY = position.y + m_worldBoundY;

        cellX = (int)(gridX / m_cellSize);
        cellY = (int)(gridY / m_cellSize);
    }

    // Since we are civilized people we will bounds-check our insertions
    bool isValidCell(int cellX, int cellY)
    {
        return cellX >= 0 && cellX < m_cellCountX && cellY >= 0 && cellY < m_cellCountY;
    }

private:
    float m_worldBoundX = 0.f;
    float m_worldBoundY = 0.f;

    float m_cellSize = 0.f;

    int m_cellCountX = 0;
    int m_cellCountY = 0;

    // Grid data structure.
    // Logically a two-dimensional array of caches,
    // where the index of a cell (x, y) is given by
    // index = y * m_cellCountX + x.
    std::vector<std::vector<T>> m_grid;
};

}