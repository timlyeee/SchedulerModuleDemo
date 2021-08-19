/****************************************************************************
 Copyright (c) 2021 Xiamen Yaji Software Co., Ltd.

 http://www.cocos.com

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated engine source code (the "Software"), a limited,
 worldwide, royalty-free, non-assignable, revocable and non-exclusive license
 to use Cocos Creator solely to develop games on your target platforms. You shall
 not use Cocos Creator software for developing other software or tools that's
 used for developing games. You are not granted to publish, distribute,
 sublicense, and/or sell copies of Cocos Creator.

 The software or tools in this License Agreement are licensed, not sold.
 Xiamen Yaji Software Co., Ltd. reserves all rights not expressly granted to you.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
****************************************************************************/
#include "core/Scheduler.h"
#include <iostream>
namespace {
constexpr uint32_t CC_REPEAT_FOREVER{UINT_MAX - 1};
constexpr uint32_t MAX_FUNC_TO_PERFORM{30};
constexpr uint32_t INITIAL_TIMER_COUND{10};
constexpr uint32_t MAX_POOL_SIZE{ 20 };
} // namespace

namespace cc {

    void Timer::setupTimerWithInterval(float seconds, unsigned int repeat, float delay) {
        _elapsed = -1;
        _interval = seconds;
        _delay = delay;
        _useDelay = _delay > 0.0F;
        _repeat = repeat;
        _runForever = _repeat == CC_REPEAT_FOREVER;
    }

    void Timer::update(float dt) {
        if (_elapsed == -1) {
            _elapsed = 0;
            _timesExecuted = 0;
            return;
        }

        // accumulate elapsed time
        _elapsed += dt;

        // deal with delay
        if (_useDelay) {
            if (_elapsed < _delay) {
                return;
            }
            trigger(_delay);
            _elapsed = _elapsed - _delay;
            _timesExecuted += 1;
            _useDelay = false;
            // after delay, the rest time should compare with interval
            if (!_runForever && _timesExecuted > _repeat) { //unschedule timer
                cancel();
                return;
            }
        }

        // if _interval == 0, should trigger once every frame
        float interval = (_interval > 0) ? _interval : _elapsed;
        while (_elapsed >= interval) {
            trigger(interval);
            _elapsed -= interval;
            _timesExecuted += 1;

            if (!_runForever && _timesExecuted > _repeat) {
                cancel();
                break;
            }

            if (_elapsed <= 0.F) {
                break;
            }

            if (_scheduler->isCurrentTimerSalvaged()) {
                break;
            }
        }
    }

    // TimerTargetCallback

    bool TimerTargetCallback::initWithCallback(Scheduler* scheduler, const ccSchedulerFunc& callback, ISchedulable* target, const std::string& key, float seconds, uint32_t repeat, float delay) {
        _scheduler = scheduler;
        _target = target;
        _callback = callback;
        _key = key;
        setupTimerWithInterval(seconds, repeat, delay);
        return true;
    }

    void TimerTargetCallback::trigger(float dt) {
        if (_callback) {
            _callback(dt);
        }
    }

    void TimerTargetCallback::cancel() {
    }

    /***** List Entry *****/
    std::vector<ListEntry*> ListEntry::_listEntries = std::vector<ListEntry*>();

    ListEntry::ListEntry(ISchedulable* target, 
        Priority priority, 
        bool paused, 
        bool markedForDeletion) :
        _target(target),
        _priority(priority),
        _markedForDeletion(markedForDeletion),
        _paused(paused){}
    ListEntry::~ListEntry() {
        delete _target;
    }
    
    ListEntry* ListEntry::getFromPool(ISchedulable* target,
        Priority priority, 
        bool paused, 
        bool markedForDeletion) {
        if (!_listEntries.empty()) {
            ListEntry* result = _listEntries.back();
            _listEntries.pop_back();
            delete result->_target;
            result->_target = target;
            result->_priority = priority;
            result->_paused = paused;
            result->_markedForDeletion = markedForDeletion;
            return result;
        }
        else {
            ListEntry* result = new ListEntry(target, priority, paused, markedForDeletion);
            return result;
        }
    }
    void ListEntry::pushToPool(ListEntry* entry) {
        if (_listEntries.size() < MAX_POOL_SIZE) {
            delete entry->_target;
            entry->_target = nullptr;
            _listEntries.push_back(entry);
        }
    }

    /**** HashUpdateEntry ****/

    std::vector<HashUpdateEntry*> HashUpdateEntry::_hashUpdateEntries = std::vector<HashUpdateEntry*>();

    HashUpdateEntry::HashUpdateEntry(void* list, 
        ListEntry* entry, 
        ISchedulable* target, 
        ccSchedulerFunc& callback) :
        _list(list),
        _entry(entry),
        _target(target),
        _callback(callback){}

    HashUpdateEntry::~HashUpdateEntry() {
        release();
    }
    HashUpdateEntry* HashUpdateEntry::getFromPool(void* list,
        ListEntry* entry,
        ISchedulable* target,
        ccSchedulerFunc& callback) {
        if (!_hashUpdateEntries.empty()) {
            HashUpdateEntry* result = _hashUpdateEntries.back();
            _hashUpdateEntries.pop_back();
            result->release();
            result->_list = list;
            result->_entry = entry;
            result->_target = target;
            result->_callback = callback;
            return result;
        } else {
            return new HashUpdateEntry(list, entry, target, callback);
        }
        
    }
    void HashUpdateEntry::pushToPool(HashUpdateEntry* entry) {
        if (_hashUpdateEntries.size() < MAX_POOL_SIZE) {
            entry->release();
            entry->_callback = nullptr;
            _hashUpdateEntries.push_back(entry);
        }
    }
    void HashUpdateEntry::release() {
        delete _list, _entry, _target;
    }
    /**** HashTimerEntry ****/
    std::vector<HashTimerEntry *> HashTimerEntry::_hashTimerEntries = std::vector<HashTimerEntry *>();
    HashTimerEntry::HashTimerEntry(std::vector<Timer*>& timers,
        ISchedulable* target,
        uint32_t timerIndex,
        Timer* currentTimer,
        bool currentTimerSalvaged,
        bool paused) :
        _target(target),
        _currentTimer(currentTimer),
        _timerIndex(timerIndex),
        _currentTimerSalvaged(currentTimerSalvaged),
        _paused(paused),
        _timers(timers){}
    HashTimerEntry::~HashTimerEntry() {
        release();
    }
    void HashTimerEntry::release() {

        _currentTimer = nullptr;// _currentTimer get from _timers logically, so it would be released with _timers
        delete _target;
        for (Timer* t : _timers)
        {
            delete t;
        }
        _timers.clear();
        
    }
    HashTimerEntry* HashTimerEntry::getFromPool(std::vector<Timer*>& timers,
        ISchedulable* target, 
        uint32_t timerIndex, 
        Timer* currentTimer, 
        bool currentTimerSalvaged, 
        bool paused) {
        if (!_hashTimerEntries.empty()) {
            //get from vector pool
            auto result = _hashTimerEntries.back();
            _hashTimerEntries.pop_back();
            result->release();
            result->_timers = timers;
            result->_currentTimer = currentTimer;
            result->_timerIndex = timerIndex;
            result->_currentTimerSalvaged = currentTimerSalvaged;
            result->_paused = paused;
            return result;
        } else { 
            return new HashTimerEntry();
        }
    }
    void HashTimerEntry::pushToPool(HashTimerEntry* entry) {
        _hashTimerEntries.push_back(entry);
    }
    /***** Scheduler *****/



    //Scheduler::Scheduler() {}
    

} // namespace cc

