
// Motor calibration
// 183mm = 3000 steps
#define STEPS_PER_MM (3000.0 / 183.0)
#define UNITS_PER_STEP (1.0 / STEPS_PER_UNIT)

// Kinematics
#define WIDTH_MM 117.0
#define LENGTH_MM 125.0

// For conditioning reasons, keep state in units of cm
#define MM_PER_UNIT 1.0
#define WIDTH_UNIT (WIDTH_MM / MM_PER_UNIT)
#define LENGTH_UNIT (LENGTH_MM / MM_PER_UNIT)
#define STEPS_PER_UNIT (STEPS_PER_MM * MM_PER_UNIT)
