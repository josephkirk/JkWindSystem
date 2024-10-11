# Real time Wind System for Unreal Engine's Open World Game
## Project Overview:
- The project focuses on implementing a real-time wind simulation in a 3D open-world game called Veloren.
- The main research question is about the performance cost of such a simulation in terms of frame rate.
## Wind Simulation Algorithm:
- Based on Jos Stam's method for solving Navier-Stokes equations.
- Uses a grid-based approach with steps including adding sources, diffusion, advection, and projection.
- Designed to be stable and fast enough for real-time use, focusing on visual results over physical accuracy.
## Key Design Considerations:
- Grid size: Affects spatial resolution and performance.
- Simulation frequency: How often the entire simulation is updated.
- World boundaries: Handling edges of the simulation grid and interactions with objects.
## Performance Evaluation:
- Measured using the Unreal Insight profiling tool.
- Main metrics: Mean and median FPS, percentage of total server time.
- Tested various grid sizes and two simulation frequencies (once per tick and once per second).
- Result on pure CPU:
    -  **Grid Size:** 256 x 256 x 256
    -  **Cell Size:** 3.91 meters
    -  **Simulated Volume:** 1000.00 meters cubed
    -  **Number of Wind Sources:** 10
    -  **Average Time per Tick:** 4.7093 ms
    -  **Total cells:** 16777216
## Current Limitation:
- Server-side: Performance cost increases with grid size, especially when run every tick.
- Running simulation once per second significantly reduced performance impact.
- Client-side: No significant performance impact detected.
- Lower simulation frequency resulted in less smooth wind behavior from the player's perspective.
- Challenges and Limitations:
- Grid size significantly affects performance due to the nature of the algorithm.
- Trade-off between simulation accuracy/smoothness and performance cost.
## Future Work and Potential:
- Optimizations like splitting the update process or using asynchronous updates.
- Interpolation for smoother transitions at lower update frequencies.
- Potential for new gameplay features like sailing ships, improved particle effects, etc.
