#pragma once

struct Level;

struct LevelGenerator {
    virtual ~LevelGenerator() = default;
    virtual void generate(Level &level) = 0;
};

struct DefaultLevelGenerator : public LevelGenerator {
    void generate(Level &level) override;
};
