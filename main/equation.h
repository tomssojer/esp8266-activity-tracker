#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define HEIGHT 180
#define WEIGHT 80

/*
    If the changes in acceleration are too small, the step counter will discard them
    A step is defined if there is a negative slope of an acceleration plot
*/
class Equation
{
public:
    // Returns the array of accelerations with the highest values
    float *get_biggest_acc(float *x_dir, float *y_dir, float *z_dir)
    {

        int x, y, z;
        for (int i = 0; i < sizeof(x_dir); i++)
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

    int calc_steps(float *data,int time)
    {
        int steps = 0;
        // float *averaged_data = smoothen_signal(data);
        float treshold = dynamic_treshold(data);
        float time1=0;
        for (int i = 0; i < sizeof(data) - 1; i++)
        {
            float sample_old = data[i];
            float sample_new = data[i + 1];
            // Add a step when the new data is smaller than old and compared to treshold
            if (sample_new < sample_old && treshold < sample_old && sample_new < treshold)
              time1 = i*time -time1
              if(time1 > 20 && time1 < 2000 ) { //TO -DO NAPAKA PRI ZAZNAVI ZARADI OKEN UPOŠTEVAJ |----.|.----| KJER JE | OKNO, . KORAK, - NIC
                steps++;
              }
        }

        return steps;
    }

    float calc_speed(int steps_per_2s)
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
        int size = sizeof(averaged_data);

        for (int i = 0; i < size; i++)
            average += averaged_data[i];

        average /= size;

        return average;
    }

    float *smoothen_data(float *data)
    {
        float averaged_data[sizeof(data) / 3 + 1];

        for (int i = 0; i < sizeof(averaged_data); i += 3)
            averaged_data[i] = (data[i] + data[i + 1] + data[i + 2]) / 3;

        return averaged_data;
    }

    int filter_steps(int steps, float *intervals)
    {
        // Filter data if the interval is less than 0.2s and more than 2s
        // intervals je tabela casov med 2 korakoma
        for (int i = 1; i < steps; i++)
        {
          float razlika_korakov = intervals[i] - intervals[i-1];
          if(razlika_korakov < 0.2 || razlika_korakov > 2) {
            steps--;
          }
        }

        return steps;
    }
};