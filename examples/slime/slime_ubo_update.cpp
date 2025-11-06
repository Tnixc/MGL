    // Create uniform buffer for compute shader parameters
    GLuint computeParamsUBO;
    glGenBuffers(1, &computeParamsUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, computeParamsUBO);
    
    struct ComputeParams {
        uint32_t width;
        uint32_t height;
        uint32_t numAgents;
        float moveSpeed;
        float deltaTime;
        float padding[3]; // Padding for std140 alignment
    };
    
    ComputeParams computeParams;
    computeParams.width = width;
    computeParams.height = height;
    computeParams.numAgents = NUM_AGENTS;
    computeParams.moveSpeed = MOVE_SPEED;
    computeParams.deltaTime = 0.016f;
    
    glBufferData(GL_UNIFORM_BUFFER, sizeof(ComputeParams), &computeParams, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 2, computeParamsUBO);
    
    // Create uniform buffer for decay shader parameters  
    GLuint decayParamsUBO;
    glGenBuffers(1, &decayParamsUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, decayParamsUBO);
    
    struct DecayParams {
        uint32_t width;
        uint32_t height;
        float decayRate;
        float padding; // Padding for std140 alignment
    };
    
    DecayParams decayParams;
    decayParams.width = width;
    decayParams.height = height;
    decayParams.decayRate = DECAY_RATE;
    
    glBufferData(GL_UNIFORM_BUFFER, sizeof(DecayParams), &decayParams, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 2, decayParamsUBO);
