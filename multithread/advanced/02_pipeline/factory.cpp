#include "factory.hpp"

#include "bounded_queue.hpp"

#include <thread>
#include <utility>

void run_factory(int n_gadgets, std::size_t queue_capacity, std::vector<Gadget>& packed) {
    BoundedQueue<Gadget> to_paint(queue_capacity);
    BoundedQueue<Gadget> to_pack(queue_capacity);

    std::thread stamp_line([&] {
        for (int i = 0; i < n_gadgets; ++i) {
            Gadget g;
            g.serial = i;
            g.stamped = true;
            to_paint.push(std::move(g));
        }
        to_paint.close();
    });

    std::thread paint_line([&] {
        Gadget g;
        while (to_paint.pop(g)) {
            g.painted = true;
            to_pack.push(std::move(g));
        }
        to_pack.close();
    });

    std::thread pack_line([&] {
        Gadget g;
        while (to_pack.pop(g)) {
            packed.push_back(std::move(g));
        }
    });

    stamp_line.join();
    paint_line.join();
    pack_line.join();
}
