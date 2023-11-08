#include "callback.h"
#include "esp32-hal-log.h"

namespace tools {
#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"

    void task_callback(void *pv_parameters) {
        Callback *callback = (Callback *) pv_parameters;
        tools::Callback::call_value_t value;

        for (;;) {
            if (xQueueReceive(callback->queue_callback, &value, portMAX_DELAY) == pdTRUE) {
                callback->call_items(value);
            }
        }
    }

#pragma clang diagnostic pop

    Callback::Callback(size_t size) {
        queue_callback = xQueueCreate(CALLBACK_BUFFER_NUM, size);
        log_i("Queue callback created");

        xTaskCreate(&task_callback, "CALLBACK", 1024, this, 15, &task_callback_call);
        log_i("Task callback created");
    }

    Callback::~Callback() {
        vTaskDelete(task_callback_call);
        log_i("Task callback deleted");
        vQueueDelete(queue_callback);
        log_i("Queue callback deleted");

        if (items) delete items;
    }

    bool Callback::init(int8_t num) {
        if (num_items > 0 || num <= 0) return false;

        if (num > CALLBACK_ITEM_MAX) num = CALLBACK_ITEM_MAX;
        num_items = num;
        items = new item_t[num];
        clear();

        log_i("Callback initialized");
        return true;
    }

    int8_t Callback::set(event_send_t item, void *p_parameters, bool only_index) {
        if (num_items > 0) {
            item_t *_item;
            for (int8_t i = 0; i < num_items; i++) {
                _item = &items[i];
                if (!_item->p_item) {
                    _item->p_item = item;
                    _item->p_parameters = p_parameters;
                    _item->only_index = only_index;

                    log_d("A callback with index %d was recorded", i);
                    return i;
                }
            }
            log_d("There is no free cell to record the callback");
        } else
            log_d("The object is not initialized");

        return -1;
    }

    void Callback::clear() {
        if (num_items > 0) {
            item_t *_item;
            for (int8_t i = 0; i < num_items; i++) {
                _item = &items[i];
                _item->p_item = nullptr;
                _item->p_parameters = nullptr;
                _item->only_index = false;
            }
            log_d("Clearing all cells");
        } else
            log_d("The object is not initialized");
    }

    void Callback::call(call_value_t &value) {
        if (num_items > 0) {
            xQueueSend(queue_callback, &value, 0);

            log_d("Calling the callback function");
        } else
            log_d("The object is not initialized");
    }

    void Callback::call_items(call_value_t &value) {
        item_t *_item;
        size_t size;

        for (int8_t i = 0; i < num_items; i++) {
            _item = &items[i];
            if (_item->p_item && (!_item->only_index || _item->only_index && value.index == i)) {
                size = _item->p_item(&value, _item->p_parameters);
                if (size != 0 && cb_receive) cb_receive(&value, p_receive_parameters);
            }
        }
    }
}
