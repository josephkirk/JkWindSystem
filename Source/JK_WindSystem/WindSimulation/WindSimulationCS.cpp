#include "WindSimulationCS.h"
#include "RenderGraphUtils.h" // For FClearUnorderedAccessViewUint

// This is what links the C++ class to the shader file.
// The first parameter is the C++ class name.
// The second is the path to the shader file relative to the /Engine/Shaders/ or your project's /Shaders/ directory.
IMPLEMENT_GLOBAL_SHADER(FWindPressureSolveCS, "/JK_WindSystem/WindPressureSolveCS.usf", "MainCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FWindPressureApplyCS, "/JK_WindSystem/WindPressureApplyCS.usf", "MainCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FAdvanceWindSimulationCS, "/JK_WindSystem/WindSimulationPCS.usf", "MainCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FVelocityInjectionCS, "/JK_WindSystem/VelocityInjection.usf", "MainCS", SF_Compute);

