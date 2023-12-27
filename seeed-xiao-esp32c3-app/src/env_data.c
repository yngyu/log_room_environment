#include "env_data.h"

bool validate_env_data(const EnvData* env_data)
{
    // BME680 range
    if (env_data->temperature < -40.0 || 85.0 < env_data->temperature) {
        return false;
    }
    if (env_data->humidity < 0.0 || 100.0 < env_data->humidity) {
        return false;
    }
    if (env_data->pressure < 30000.0 || 110000.0 < env_data->pressure) {
        return false;
    }
    // MH Z19C range
    if (env_data->co2_ppm < 300.0 || 10000.0 < env_data->co2_ppm) {
        return false;
    }

    return true;
}
