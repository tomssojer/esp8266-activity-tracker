#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <float.h>
#include <math.h>
#include "constants.h"

class Equation
{
public:
    uint16_t time_last = 0;
    // Returns the array of accelerations with the highest values
   float *get_highest_acc(float *x_dir, float *y_dir, float *z_dir)
    {
        int x=0;
        int y=0;
        int z=0;
        
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
        {
            return smooth_data(x_dir);
        }

        else if (y >= z)
        {
            return smooth_data(y_dir);
        }

        return smooth_data(z_dir);
    }

    float *get_magnitude(float *x_dir, float *y_dir, float *z_dir) // vektor dolzine Size za magnitude, neodvisno od rotacije
    {
        // returns smooteth/filtered magnitudes of x,y,z accerelations, using moving avrage filter -> low_pass filter
        float *mag = (float *)malloc(SIZE * sizeof(float));
        if (mag == NULL)
        {
            printf("MEMORY ERROR MAGNITUDE\n"); // Handle memory allocation error
        }
        for (int i = 0; i < SIZE; i++)
        {
            mag[i] = sqrt(x_dir[i] * x_dir[i] + y_dir[i] * y_dir[i] + z_dir[i] * z_dir[i]);
            // printf("%f ", mag[i]);
        }
        float *smooth = low_pass_filter(mag,WINDOWS_SIZE);
        //float *smooth = smooth_data(mag); // za classic uporabi kar to ali zgornjo z 2
        free(mag);
        return smooth;
    }

    uint8_t calc_steps(float *data, uint16_t time_old)
    {
        uint8_t steps = 0;
        uint8_t fake_steps = 0; // le za simulacijo
        uint16_t time_new = time_last;
        //določi dinamični prag
        float treshold = dynamic_treshold_avrage(data); 

        for (int i = 0; i < SIZE - 1; i++)
        {
            float sample_old = data[i];
            float sample_new = data[i + 1];
            // Add a step when the new data is smaller than old and compared to treshold
            if ((sample_new < sample_old) && (treshold < sample_old) && (sample_new < treshold))
            {
                uint16_t time_temp = (uint16_t)i * time_old - time_new;
                if (time_temp > MIN_TIME_STEP)
                { 
                    steps++;
                    time_new = (uint16_t)i * time_old;
                }
                else
                    fake_steps++; // le za simulacijo
            }
        }
        uint16_t timeDiff = SIZE * time_old - time_last;
        if (timeDiff > MIN_TIME_STEP) {
           time_last = 0;
        } 
        else {
          time_last = timeDiff;
        }
        //printf("The number of steps: %d\n", steps);
        //printf("The number of fake steps: %d\n", fake_steps);
        return steps;
    }
   uint8_t calc_steps_deriv(float *data, uint16_t time_old)
    {
        uint8_t steps = 0;
        uint8_t fake_steps = 0;
        uint16_t time_new = time_old;
        float prevDerivative = 0.0;
  
        for (int i = 1; i < SIZE; i++)
        {
          float currentDerivative = data[i] - data[i - 1];
            // Add a step when the new data is smaller than old and compared to treshold, ta je fiksen, na 1 decimalko da se izloci sum
            if (((currentDerivative > 0 && prevDerivative < 0) || (currentDerivative < 0 && prevDerivative > 0)) && fabs(currentDerivative)>DERIVATIVE_TRESHOLD)
            {
                uint16_t time_temp = (uint16_t)i * time_old - time_new;
                if (time_temp > MIN_TIME_STEP)
                {
                    steps++;
                    time_new = (uint16_t)i * time_old;
                }
                else 
                 {
                   fake_steps++;
                 }     
            }
            prevDerivative = currentDerivative;
        }
        uint16_t timeDiff = SIZE * time_old - time_last;
        // Upoštevamo tudi čase med dvema oknoma-50 vzorcih. Če se detekcija pojavi na 49 indeksu in na prvem indeksu v naslednjem oknu ni ok
        if (timeDiff > MIN_TIME_STEP) {
          time_last = 0;
        }  
        else {
          time_last = timeDiff;
        } 
        //printf("The number of steps: %d\n", steps);
        //printf("The number of fake steps: %d\n", fake_steps);
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

    // Calculates the tresholds, 2 methods
    float dynamic_treshold_avrage(float *averaged_data)
    {
        float treshold;
        float average;

        for (int i = 0; i < SIZE; i++)
            average += averaged_data[i];

        average /= SIZE;

        return average;
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
    // filters for data smoothing 
    float *smooth_data(float *data) //Glajenje s povprečje vseh vzorcev - isto kot spodnji z windows_length 3
    {
        // float averaged_data[SIZE];
        float *averaged_data = (float *)malloc(SIZE * sizeof(float));
        // padding na zacetku in koncu, da ohranimo dolzino signala
        averaged_data[0] = (data[0] + data[1]) / 2;
        averaged_data[SIZE - 1] = (data[SIZE - 2] + data[SIZE - 1]) / 2;

        for (int i = 1; i < SIZE - 2; i++)
        {
            averaged_data[i] = (data[i - 1] + data[i] + data[i + 1]) / 3; // filter signala - povprečje prejsnje, zdasnje naslednj
        }

        return averaged_data;
    }

    float *low_pass_filter(float* input, int windowSize) { //Glajenje s povprečje vseh vzorcev v oknu
        int halfWindowSize = windowSize / 2;
        
        float *output = (float *)malloc(SIZE * sizeof(float));
        for (int i = 0; i < SIZE; i++) {
            float sum = 0.0;
            int count = 0;
            for (int j = i - halfWindowSize; j <= i + halfWindowSize; j++) {
                if (j >= 0 && j < SIZE) {
                    sum += input[j];
                    count++;
                }
            }
            output[i] = sum / count;
        }
        return output;
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