#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <float.h>
#include <math.h>

#define HEIGHT 180
#define WEIGHT 80
#define SIZE 50
#define MIN_TIME_STEP 200
/*
    If the changes in acceleration are too small, the step counter will discard them
    A step is defined if there is a negative slope of an acceleration plot
*/
class Equation
{
public:
    // int SIZE;

    // void setSize(int size)
    // {
    //     SIZE = size;
    //     printf("Size: %d\n", SIZE);
    // }
    uint16_t time_last = 0;
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
        {
            printf("X\n");
            return x_dir;
        }

        else if (y >= z)
        {
            printf("Y\n");
            return y_dir;
        }

        printf("Z\n");
        return z_dir;
    }

    float *get_magnitude(float *x_dir, float *y_dir, float *z_dir) // vektor dolzine Size za magnitude, neodvisno od rotacije
    {
        // returns smooteth/filtered magnitudes of x,y,z accerelations, using avrage filter
        float *mag = (float *)malloc(SIZE * sizeof(float));
        if (mag == NULL)
        {
            printf("MEMORY ERROR MAGNITUDE\n"); // Handle memory allocation error
            // ...
        }
        // printf("size is: %d values x: %f y: %f z: %f  \n", SIZE, x_dir[49], y_dir[49], z_dir[49]);
        for (int i = 0; i < SIZE; i++)
        {
            mag[i] = sqrt(x_dir[i] * x_dir[i] + y_dir[i] * y_dir[i] + z_dir[i] * z_dir[i]);
            // printf("%f ", mag[i]);
        }

        float *smooth = low_pass_filter(mag,4);
        //float *smooth = smooth_data(mag);
        // printf("%f\n", mag[0]);
        free(mag);

        return smooth;
    }

    uint8_t calc_steps(float *data, uint16_t time_old)
    {
        uint8_t steps = 0;
        uint8_t fake_steps = 0;

        // Serial.println("Func call");
        //float *averaged_sdata = smoothen_signal(data);
        float treshold = dynamic_treshold(data);
        // printf("THRESHOLD: %f ------", treshold);
        uint16_t time_new = time_last;

        for (int i = 0; i < SIZE - 1; i++)
        {
            float sample_old = data[i];
            float sample_new = data[i + 1];

            // printf("%d | %f | %f\n", i, sample_old, sample_new);

            // Add a step when the new data is smaller than old and compared to treshold
            if ((sample_new < sample_old) && (treshold < sample_old) && (sample_new < treshold))
            {
                uint16_t time_temp = (uint16_t)i * time_old - time_new;

                // Serial.println("step");
                if (time_temp > MIN_TIME_STEP)
                { // TO -DO NAPAKA PRI ZAZNAVI ZARADI OKEN UPOŠTEVAJ |----.|.----| KJER JE | OKNO, . KORAK, - NIC
                    steps++;
                    // printf("%d | %d\n", time_temp, time_new);
                    time_new = (uint16_t)i * time_old;
                }
                else
                    fake_steps++;
            }
        }
          uint16_t timeDiff = SIZE * time_old - time_last;

        if (timeDiff > MIN_TIME_STEP)
            time_last = 0;
        else
            time_last = timeDiff;

        printf("The number of steps: %d\n", steps);
        printf("The number of fake steps: %d\n", fake_steps);

        return steps;
    }
   uint8_t calc_steps_deriv(float *data, uint16_t time_old)
    {
        uint8_t steps = 0;
         uint8_t fake_steps = 0;

        uint16_t time_new = time_old;
        float prevDerivative = 0.0;
        float treshold = dynamic_treshold(data);
        for (int i = 1; i < SIZE; i++)
        {
          float currentDerivative = data[i] - data[i - 1];
            //printf("currentDerivative %f  || %f \n", currentDerivative,prevDerivative);

            // printf("%d | %f | %f\n", i, sample_old, sample_new);

            // Add a step when the new data is smaller than old and compared to treshold
            if (((currentDerivative > 0 && prevDerivative < 0) || (currentDerivative < 0 && prevDerivative > 0)) && fabs(currentDerivative)>0.1)
            {
                uint16_t time_temp = (uint16_t)i * time_old - time_new;
               
                // Serial.println("step");
                if (time_temp > MIN_TIME_STEP)
                {
                   //printf("time: %d | %d \n",time_temp,time_new);
                    steps++;
                    // printf("%d | %d\n", time_temp, time_new);
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

        if (timeDiff > MIN_TIME_STEP)
            time_last = 0;
        else
            time_last = timeDiff;

    

        printf("The number of steps: %d\n", steps);
        printf("The number of fake steps: %d\n", fake_steps);
      

        return steps;
    }

    float calc_speed(uint16_t steps_per_2s)
    {
        // Interval is 0.04 s
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
        else if (steps_per_2s <= 8)
            stride = HEIGHT;
        else
            stride = 1.2 * HEIGHT;

        float speed = steps_per_2s * stride;

        return speed;
    }

    // Calories calculated per 2 seconds
    float calc_calories(float speed)
    {
        float calories = speed * WEIGHT / 400;

        return calories;
    }

private:
    // Calculates the treshold in one of the directions
    float dynamic_treshold(float *averaged_data)
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
    //filtri 
    float *smooth_data(float *data) //Glajenje s povprečje vseh vzorcev
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
            //printf("Povp_ %f \n",output[i]);
        }
        return output;
    }

    float *calculate_derivative(float* input) { //aproksimacija odvodov
      float *output = (float *)malloc(SIZE * sizeof(float));
      output[0] = input[1] - input[0];
      for (int i = 1; i < SIZE; i++) {
          output[i] = input[i] - input[i - 1];
          printf(":::: %f\n", output[i]);
      }
      return output;      
}

    uint16_t filter_steps(uint16_t steps, float *intervals)
    {
        // Filter data if the interval is less than 0.2s and more than 2s
        // intervals je tabela casov med 2 korakoma
        for (int i = 1; i < steps; i++)
        {
            float razlika_korakov = intervals[i] - intervals[i - 1];
            if (razlika_korakov < 0.2 || razlika_korakov > 2)
            {
                steps--;
            }
        }

        return steps;
    }
};