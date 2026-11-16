API_EXPORT void Set_R2Y_Config_packed(IN const R2Y_User_Config *user_config,
    OUT R2Y_SW_Config *R2Y_Config)
{
    R2Y_User_Config defaults;
    int channel;
    if (user_config == NULL || R2Y_Config == NULL) return;
    defaults = *user_config;
    for (channel = 0; channel < 3; ++channel) {
        defaults.kb[channel] = 0.0f;
        defaults.kc[channel] = 1.0f;
    }
    defaults.ks[0] = defaults.ks[1] = 1.0f;
    (void)Set_R2Y_BSC_Config(&defaults, R2Y_Config);
}