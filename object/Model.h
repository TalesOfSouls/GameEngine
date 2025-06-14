
// Memory layout TBD, we can probably optimize it for matrix operations

///////////////////////////////
// Model
// ============================
// Mesh
// ============================
// Current Transform/Mesh
// ============================
// Joint 1
// ============================
// Child Joint Id 1
// Child Joint Id 2
// ...
// ============================
// Joint 2
// ....
// ============================
// Animation 1
// ============================
// Keyframe 1
// ============================
// Joint 1 Transform
// Joint 2 Transform
// ....
// ============================
// Keyframe 2
// ============================
// ....
// ============================
// Animation 2
// ============================
// ....
// ============================

struct Model {
    Mesh* mesh;
};

struct ModelJoint {
    byte children_count;
};

struct ModelAnimation {
    // joint positions
};