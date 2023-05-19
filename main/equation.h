#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <float.h>

#define HEIGHT 180
#define WEIGHT 80

/*
    If the changes in acceleration are too small, the step counter will discard them
    A step is defined if there is a negative slope of an acceleration plot
*/
class Equation
{
public:
    int SIZE;

    void setSize(int size)
    {
        SIZE = size;
        printf("Size: %d\n", SIZE);
    }

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

    uint8_t calc_steps(float *data, uint16_t time_old)
    {
        uint8_t steps = 0;
        // Serial.println("Func call");
        //  float *averaged_sdata = smoothen_signal(data);
        float treshold = dynamic_treshold(data);
        uint16_t time_new = 0;

        for (int i = 0; i < SIZE - 1; i++)
        {
            float sample_old = data[i];
            float sample_new = data[i + 1];

            // Add a step when the new data is smaller than old and compared to treshold
            if (sample_new < sample_old && treshold < sample_old && sample_new < treshold)
            {
                uint16_t time_temp = i * time_old - time_new;
                // Serial.println("step");
                steps++;
                if (time_new > 200)
                { // TO -DO NAPAKA PRI ZAZNAVI ZARADI OKEN UPOŠTEVAJ |----.|.----| KJER JE | OKNO, . KORAK, - NIC

                    time_new = time_temp;
                }
            }
        }

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
            {
                min = averaged_data[i];
            }
            if (averaged_data[i] > max)
            {
                max = averaged_data[i];
            }
        }
        float average = (min + max) / 2;

        return average;
    }

    float *smooth_data(float *data)
    {
        // Upoštevaj manjšo velikost od SIZE !!!!!!!
        float averaged_data[SIZE - 2];
        int size = SIZE - 2; // sizeof(averaged_data);

        averaged_data[0] = (data[0] + data[1]) / 2; // padding na zacetku in koncu, da ohranimo dolzino signala
        averaged_data[0] = (data[SIZE - 2] + data[SIZE - 1]) / 2;
        for (int i = 1; i < size - 1; i++)
        {
            averaged_data[i] = (data[i - 1] + data[i] + data[i + 1]) / 3; // filter signala - povprečje prejsnje, zdasnje naslednj
        }

        return averaged_data;
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