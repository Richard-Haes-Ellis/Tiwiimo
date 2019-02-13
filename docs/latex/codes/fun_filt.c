int32_t filter(int32_t sensVal, int32_t values[N])
{
	int8_t i = 0;
	int8_t j = 0;
	int32_t avg;

	// Desplazamos todos los elementos a la izquierda
	for (i = 0; i < N - 1; i++)
	{
		values[i] = values[i + 1];
	}

	// Introducimos la medida al ultimo elemento del vector
	values[N - 1] = sensVal;

	// Calculamos el valor medio de todos los elementos del vector
	avg = 0;
	for (i = 0; i < N; i++)
	{
		avg = avg + values[i];
	}
	return avg / N; // Dividimos para la media
}
