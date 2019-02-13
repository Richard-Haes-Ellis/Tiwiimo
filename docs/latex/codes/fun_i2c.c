// Funcion para testear el sensor del sensorpack
uint8_t Test_I2C_dir(uint32_t pos, uint8_t dir)
{
	uint8_t  error = 100;
	uint32_t I2C_Base = 0;
	uint32_t *I2CMRIS;

	// En funcion de donde tengamos el sensorpack
	if(pos==1) I2C_Base=I2C0_BASE;
	if(pos==2) I2C_Base=I2C2_BASE;

	if(I2C_Base)
	{
		I2CMasterSlaveAddrSet(I2C_Base, dir, 1);  //Modo LECTURA
		I2CMasterControl(I2C_Base, I2C_MASTER_CMD_SINGLE_RECEIVE); //Lect. Simple

		while(!(I2CMasterBusy(I2C_Base)));  //Espera que empiece
		while((I2CMasterBusy(I2C_Base)));   //Espera que acabe

		I2CMRIS= (uint32_t *)(I2C_Base+0x014);
		error=(uint8_t)((*I2CMRIS) & 0x10);
		if(error)
		{
			I2CMRIS=(uint32_t *)(I2C_Base+0x01C);
			*I2CMRIS=0x00000010;
		}
	}
	return error;
}
