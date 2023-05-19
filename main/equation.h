#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <float.h>
#include <math.h>
#include "constants.h"

uint16_t time_last = 0;

class Equation
{
public:
    // Returns the array of accelerations with the highest values
    float *get_highest_acc(float *x_dir, float *y_dir, float *z_dir)
    {
        int x, y, z;
        for (int i = 0; i < SIZE; i++)
        {
            float x_abs = abs(x_dir[i]);
            float y_abs = abs(y_dir[i]);
            float z_abs = abs(z_dir[i]);

            if (x_abs >= y_abs && x_abs >= z_abs)
                x++;
            else if (y_abs >= z_abs)
                y++;
            else
                z++;
        }

        // Compare counts from the arrays
        if (x >= y && x >= z)
            return x_dir;
        else if (y >= z)
            return y_dir;

        return z_dir;
    }

    // Vektor dolžine SIZE za magnitudo, je neodvisen od rotacije
    float *get_magnitude(float *x_dir, float *y_dir, float *z_dir)
    {
        // Returns filtered magnitudes of x,y,z accerelations, using average filter
        float *mag = (float *)malloc(SIZE * sizeof(float));

        if (mag == NULL)
            printf("MEMORY ERROR MAGNITUDE\n"); // Handle memory allocation error

        for (int i = 0; i < SIZE; i++)
            mag[i] = sqrt(x_dir[i] * x_dir[i] + y_dir[i] * y_dir[i] + z_dir[i] * z_dir[i]);

        float *smooth = smooth_data(mag);
        free(mag);

        return smooth;
    }

    uint8_t calc_steps(float *data, uint16_t time_old)
    {
        uint8_t steps = 0;
        uint8_t fake_steps = 0;

        float treshold = dynamic_treshold(data);
        uint16_t time_new = time_last;

        for (int i = 0; i < SIZE - 1; i++)
        {
            float sample_old = data[i];
            float sample_new = data[i + 1];

            // Add a step when the new data is smaller than old and compared to treshold
            if ((sample_new < sample_old) && (treshold < sample_old) && (sample_new < treshold))
            {
                uint16_t time_temp = (uint16_t)i * time_old - time_new;

                if (time_temp > MIN_TIME_STEP && time_temp < (SIZE * time_old))
                {
                    steps++;
                    time_new = time_temp;
                    time_last = time_new;
                }
            }
        }

        uint16_t timeDiff = SIZE * time_old - time_last;

        if (timeDiff > MIN_TIME_STEP)
            time_last = 0;
        else
            time_last = timeDiff;

        printf("The number of steps: %d\n", steps);

        return steps;
    }

    float calc_speed(uint8_t steps_per_2s)
    {
        float stride = calc_stride(steps_per_2s);
        float speed = steps_per_2s * stride / 2;

        return speed;
    }

    float calc_distance(uint8_t steps_per_2s)
    {
        float stride = calc_stride(steps_per_2s);
        float distance = stride * steps_per_2s;

        return distance;
    }

    float calc_calories(float speed)
    {
        float calories = speed * WEIGHT / 400;

        return calories;
    }

    char *infer_movement_type(u_int8_t steps_per_2s)
    {
        char *type;

        if (steps_per_2s > 4)
            type = "Tek";
        else
            type = "Hoja";

        return type;
    }

private:
    // Calculates the treshold in one of the directions
    float dynamic_treshold(float *averaged_data)
    {
        float treshold;

        for (int i = 0; i < SIZE; i++)
            treshold += averaged_data[i];

        treshold /= SIZE;

        return treshold;
    }

    float dynamic_treshold_MIN_MAX(float *averaged_data)
    {
        float treshold;
        float min = FLT_MAX;
        float max = FLT_MIN;

        for (int i = 0; i < SIZE; i++)
        {
            if (averaged_data[i] < min)
                min = averaged_data[i];

            if (averaged_data[i] > max)
                max = averaged_data[i];
        }
        float average = (min + max) / 2;

        return average;
    }

    float *smooth_data(float *data)
    {
        float *averaged_data = (float *)malloc(SIZE * sizeof(float));

        // Padding na zacetku in koncu, da ohranimo dolzino signala
        averaged_data[0] = (data[0] + data[1]) / 2;
        averaged_data[SIZE - 1] = (data[SIZE - 2] + data[SIZE - 1]) / 2;

        // Filter signala - povprečje prejšnje, zdašnje in naslednje
        for (int i = 1; i < SIZE - 2; i++)
            averaged_data[i] = (data[i - 1] + data[i] + data[i + 1]) / 3;

        return averaged_data;
    }

    float calc_stride(uint8_t steps_per_2s)
    {
        float stride;

        if (steps_per_2s <= 2)
            stride = HEIGHT / 5;
        else if (steps_per_2s <= 3)
            stride = HEIGHT / 4;
        else if (steps_per_2s <= 4)
            stride = HEIGHT / 3;
        else if (steps_per_2s <= 5)
            stride = HEIGHT / 2;
        else if (steps_per_2s <= 6)
            stride = HEIGHT / 1.2;
        else if (steps_per_2s < 8)
            stride = HEIGHT;
        else
            stride = 1.2 * HEIGHT;

        return stride;
    }
};