// PlatformIO Unified Debugging / mbed threading demo code
// 2018, Digital Incubation and Growth GmbH, Apache License 2.0

#include <mbed.h>
#include <math.h>
#include <string.h>

struct counter_s {
    int  counter;
    char buf[32];
    Mutex m;
};

void counter_update(counter_s *s) {
    s->m.lock();

    s->counter++;
    snprintf(s->buf, sizeof(s->buf), "T=%p C=%i", s->m.get_owner(), s->counter);
    
    s->m.unlock();
}

void counter_init(counter_s *s) {
    s->m.lock();

    s->counter = 0;
    memset(s->buf, 0, sizeof(s->buf));
    
    s->m.unlock();
}


DigitalOut led(LED1);
Serial pc(USBTX, USBRX); 


// counter_thread takes the counter_s struct, waits for 
// random amount of time (0..1s) and calls counter_update to
// increment the counter value(s).
void counter_thread(void* arg) {
    while(true) {
        float f = (float)(rand())/(float)RAND_MAX;
        wait(f);

        counter_update((counter_s*)arg);
    }
}

// blink_thread runs 10x/sec, turns on onboard LED
// is counter_s' value is odd (turns off if value is even)
void blink_thread(counter_s *s) {
    while(true) {
        led = (s->counter % 2);
        wait(0.1);
    }
}

// output_thread dumps counter_s' values to USB serial
// once per second.
void output_thread(counter_s *s) {
    while(true) {
        if (s->m.trylock()) {
            pc.printf("%s\n\r", s->buf);        
            s->m.unlock();
        }
        wait(1);
    }
}

counter_s c;

// threads
const int NUM_THREADS = 10;
Thread threads[NUM_THREADS];

int main() {
    pc.baud(115200);

    counter_init(&c);

    // start LED and output thread
    threads[0].start(callback(blink_thread, &c));
    threads[1].start(callback(output_thread, &c));

    // start remaining threads as counter update threads
    for ( int i = 2; i < NUM_THREADS; i++) {
        threads[i].start(callback(counter_thread, &c));
    }
    return 0;
}