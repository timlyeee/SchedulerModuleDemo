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

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include "core/System.h"

namespace cc {

using ccSchedulerFunc = std::function<void(float)>;
class Scheduler;
/**
	 * @cond
	 */
class CC_DLL Timer {
public:
    /** get interval in seconds */
    inline float getInterval() const { return _interval; };
    /** set interval in seconds */
    inline void setInterval(float interval) { _interval = interval; };

    virtual void setupTimerWithInterval(float interval, uint32_t repeat, float delay) = 0;

    virtual void trigger(float dt) = 0;
    virtual void cancel()          = 0;

    /** triggers the timer */
    void update(float dt);
//protected dtor? Need to consider how to release space
    Timer() = default;
    virtual ~Timer() = default; 
protected:
    Scheduler* _scheduler{nullptr};
    float      _elapsed{0.f};
    bool       _runForever{false};
    bool       _useDelay{false};
    uint32_t   _timesExecuted{0};
    uint32_t   _repeat{0};
    float      _delay{0.f};
    float      _interval{0.f};
};

class CC_DLL TimerTargetCallback final : public Timer {
public:
    TimerTargetCallback() = default;
    void setupTimerWithInterval(float interval, uint32_t repeat, float delay) override;
    // Initializes a timer with a target, a lambda and an interval in seconds, repeat in number of times to repeat, delay in seconds.
    bool initWithCallback(Scheduler* scheduler, const ccSchedulerFunc& callback, ISchedulable* target, const std::string& key, float seconds, uint32_t repeat, float delay);

    inline const ccSchedulerFunc& getCallback() const { return _callback; };
    inline const std::string&     getKey() const { return _key; };

    void trigger(float dt) override;
    void cancel() override;

private:
    ISchedulable*   _target{nullptr};
    ccSchedulerFunc _callback{nullptr};
    std::string     _key;
};

/**
 * @en A list double-linked list used for "updates with priority"
 * @zh ?????????????????????????????????
 * @class ListEntry
 * @param target not retained (retained by hashUpdateEntry)
 * @param priority
 * @param paused
 * @param markedForDeletion selector will no longer be called and entry will be removed at end of the next tick
 */
class ListEntry final {
public:
    ISchedulable* _target{nullptr};
    Priority      _priority{Priority::LOW};
    bool          _paused{false};
    bool          _markedForDeletion{false};

    static ListEntry* getFromPool(ISchedulable* target, Priority priority, bool paused, bool markedForDeletion);
    static void       pushToPool(ListEntry* entry);
    ~ListEntry();
protected:
    ListEntry() {}
    ListEntry(ISchedulable* target, Priority priority, bool paused, bool markedForDeletion);
   

    
private:
    static std::vector<ListEntry*> _listEntries;
};

/**
 * @en A update entry list
 * @zh ??????????????????
 * @class HashUpdateEntry
 * @param list Which list does it belong to ?
 * @param entry entry in the list
 * @param target hash key (retained)
 * @param callback
 */
class HashUpdateEntry final {
public:
    void*           _list{nullptr}; //unknown usage
    ListEntry*      _entry{nullptr};
    ISchedulable*   _target{nullptr};
    ccSchedulerFunc _callback{nullptr};

    static HashUpdateEntry* getFromPool(void* list, ListEntry* entry, ISchedulable* target, ccSchedulerFunc& callback);
    static void             pushToPool(HashUpdateEntry* entry);
    ~HashUpdateEntry();
protected:
    HashUpdateEntry() {}
    HashUpdateEntry(void* list, ListEntry* entry, ISchedulable* target, ccSchedulerFunc& callback);
   
    void release();
private:
    static std::vector<HashUpdateEntry*> _hashUpdateEntries;
};

/**
 * @en Hash Element used for "selectors with interval"
 * @zh ???????????????????????????????????????
 * @class HashTimerEntry
 * @param timers
 * @param target  hash key (retained)
 * @param timerIndex
 * @param currentTimer
 * @param currentTimerSalvaged
 * @param paused
 */
class HashTimerEntry final {
public:
    std::vector<Timer*> _timers;
    ISchedulable*       _target{nullptr};
    uint32_t            _timerIndex{0};
    Timer*              _currentTimer{nullptr};
    bool                _currentTimerSalvaged{false};
    bool                _paused{false};

    static HashTimerEntry* getFromPool(std::vector<Timer*>& timers, ISchedulable* target, uint32_t timerIndex, Timer* currentTimer, bool currentTimerSalvaged, bool paused);
    static void            pushToPool(HashTimerEntry* entry);
    ~HashTimerEntry();
protected:
    HashTimerEntry() {}
    HashTimerEntry(std::vector<Timer*>& timers, ISchedulable* target, uint32_t timerIndex, Timer* currentTimer, bool currentTimerSalvaged, bool paused);
    
    void release();
private:
    static std::vector<HashTimerEntry*> _hashTimerEntries;
};

/**
 * @en
 * Scheduler is responsible of triggering the scheduled callbacks.<br>
 * You should not use NSTimer. Instead use this class.<br>
 * <br>
 * There are 2 different types of callbacks (selectors):<br>
 *     - update callback: the 'update' callback will be called every frame. You can customize the priority.<br>
 *     - custom callback: A custom callback will be called every frame, or with a custom interval of time<br>
 * <br>
 * The 'custom selectors' should be avoided when possible. It is faster,<br>
 * and consumes less memory to use the 'update callback'. *
 * @zh
 * Scheduler ????????????????????????????????????<br>
 * ?????????????????????????????? `director.getScheduler()` ???????????????????????????<br>
 * ????????????????????????????????????<br>
 *     - update ??????????????????????????????????????????????????????????????????<br>
 *     - ?????????????????????????????????????????????????????????????????????????????????????????????<br>
 * ?????????????????????????????????????????? update ?????????????????? update ????????????????????????????????????????????????
 *
 * @class Scheduler
 */
class Scheduler final : public System {
private:
    float                  _timeScale{1.f};
    std::vector<ListEntry*> _updatesNegList;
    std::vector<ListEntry*> _updates0List;
    std::vector<ListEntry*> _updatesPosList;

    std::unordered_map<void*, HashUpdateEntry*> _hashForUpdates;
    std::unordered_map<void*, HashTimerEntry*>  _hashForTimers;

    //Old ts code for _currentTarget, _currentTargetSalved
    HashTimerEntry*     _currentTimer{nullptr};
    bool                _currentTimerSalvaged{false};
    bool                _updateHashLocked{false};
    std::vector<Timer*> _arrayForTimers;

    //Previous: _removeHashElement, now: _removeTimerFromHash
    void _removeTimerFromHash(HashTimerEntry* element);
    void _removeUpdateFromHash(HashUpdateEntry* element);
    void _priorityIn(std::vector<ListEntry*>& pplist, ListEntry* listElement, Priority priority);
    void _appendIn(std::vector<ListEntry*>& pplist, ListEntry* listElement);

public:
    static void enableForTarget(ISchedulable* target);
    Scheduler();
    ~Scheduler();
    

    bool inline isCurrentTimerSalvaged() const { return _currentTimerSalvaged; }

    bool inline isUpdateHashLocked() const { return _updateHashLocked; }

    /**
     * @en
     * Modifies the time of all scheduled callbacks.<br>
     * You can use this property to create a 'slow motion' or 'fast forward' effect.<br>
     * Default is 1.0. To create a 'slow motion' effect, use values below 1.0.<br>
     * To create a 'fast forward' effect, use values higher than 1.0.<br>
     * Note???It will affect EVERY scheduled selector / action.
     * @zh
     * ????????????????????????????????????<br>
     * ?????????????????????????????????????????? ???slow motion?????????????????? ??? ???fast forward??????????????? ????????????<br>
     * ????????? 1.0?????????????????? ???slow motion?????????????????? ??????,??????????????? 1.0???<br>
     * ????????? ???fast forward??????????????? ???????????????????????? 1.0???<br>
     * ????????????????????? Scheduler ??????????????????????????????
     * @param timeScale
     */
    void inline setTimeScale(float t) { _timeScale = t; }
    float inline getTimeScale() const { return _timeScale; }

    /**
     * @en 'update' the scheduler. (You should NEVER call this method, unless you know what you are doing.)
     * @zh update ???????????????(????????????????????????????????????????????????????????????????????????)
     * @param dt delta time
     */
    void update(float dt);

    /**
     * @en
     * <p>
     *   The scheduled method will be called every 'interval' seconds.<br/>
     *   If paused is YES, then it won't be called until it is resumed.<br/>
     *   If 'interval' is 0, it will be called every frame, but if so, it recommended to use 'scheduleUpdateForTarget:' instead.<br/>
     *   If the callback function is already scheduled, then only the interval parameter will be updated without re-scheduling it again.<br/>
     *   repeat let the action be repeated repeat + 1 times, use `macro.REPEAT_FOREVER` to let the action run continuously<br/>
     *   delay is the amount of time the action will wait before it'll start<br/>
     * </p>
     * @zh
     * ???????????????????????????????????????????????????????????????????????????<br/>
     * ?????? paused ?????? true??????????????? resume ???????????????????????????<br/>
     * ???????????????????????????????????????????????????????????????????????????<br/>
     * ?????? interval ?????? 0?????????????????????????????????????????????????????????????????????
     * ???????????? scheduleUpdateForTarget ?????????<br/>
     * ????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????<br/>
     * repeat ??????????????????????????? repeat + 1 ???????????? `macro.REPEAT_FOREVER`
     * ???????????????????????????????????????<br/>
     * delay ?????????????????????????????????????????????????????????????????????????????????
     * @param callback
     * @param target
     * @param interval
     * @param [repeat]
     * @param [delay=0]
     * @param [paused=fasle]
     */
    void schedule(ccSchedulerFunc& callback, ISchedulable* target, uint32_t interval, uint32_t repeat, uint32_t delay, bool paused = false);

    /**
     * @en
     * Schedules the update callback for a given target,
     * During every frame after schedule started, the "update" function of target will be invoked.
     * @zh
     * ???????????????????????????????????????????????? update ????????????<br>
     * update ???????????????????????????????????????????????????????????????????????? "update" ?????????<br>
     * ??????????????????????????????????????????????????????
     * @param target
     * @param priority
     * @param paused
     */
    void scheduleUpdate(ISchedulable* target, Priority priority, bool paused);

    /**
     * @en
     * Unschedules a callback for a callback and a given target.
     * If you want to unschedule the "update", use `unscheduleUpdate()`
     * @zh
     * ??????????????????????????????
     * ?????????????????? update ????????????????????? unscheduleUpdate()???
     * @param callback The callback to be unscheduled
     * @param target The target bound to the callback.
     */
    void unschedule(ccSchedulerFunc& callback, ISchedulable* target);

    /**
     * @en Unschedules the update callback for a given target.
     * @zh ????????????????????? update ????????????
     * @param target The target to be unscheduled.
     */
    void unscheduleUpdate(ISchedulable* target);

    /**
     * @en
     * Unschedules all scheduled callbacks for a given target.
     * This also includes the "update" callback.
     * @zh ????????????????????????????????????????????? update ????????????
     * @param target The target to be unscheduled.
     */
    void unscheduleAllForTarget(ISchedulable* target);

    /**
     * @en
     * Unschedules all scheduled callbacks from all targets including the system callbacks.<br/>
     * You should NEVER call this method, unless you know what you are doing.
     * @zh
     * ???????????????????????????????????????????????????????????????<br/>
     * ?????????????????????????????????????????????????????????
     */
    void unscheduleAll();

    /**
     * @en
     * Unschedules all callbacks from all targets with a minimum priority.<br/>
     * You should only call this with `PRIORITY_NON_SYSTEM_MIN` or higher.
     * @zh
     * ???????????????????????????????????????????????????????????????<br/>
     * ??????????????????????????????????????? PRIORITY_NON_SYSTEM_MIN ???????????????
     * @param minPriority The minimum priority of selector to be unscheduled. Which means, all selectors which
     *        priority is higher than minPriority will be unscheduled.
     */
    void unscheduleAllWithMinPriority(Priority minPriority);

    /**
     * @en Checks whether a callback for a given target is scheduled.
     * @zh ????????????????????????????????????????????????????????????????????????
     * @param callback The callback to check.
     * @param target The target of the callback.
     * @return True if the specified callback is invoked, false if not.
     */
    bool isScheduled(ccSchedulerFunc& callback, ISchedulable* target);

    /**
     * @en
     * Pauses the target.<br/>
     * All scheduled selectors/update for a given target won't be 'ticked' until the target is resumed.<br/>
     * If the target is not present, nothing happens.
     * @zh
     * ?????????????????????????????????<br/>
     * ????????????????????????????????????????????????<br/>
     * ???????????????????????????????????????????????????????????????
     * @param target
     */
    void pauseTarget(ISchedulable* target);

    /**
     * @en
     * Pause all selectors from all targets.<br/>
     * You should NEVER call this method, unless you know what you are doing.
     * @zh
     * ???????????????????????????????????????<br/>
     * ???????????????????????????????????????????????????????????????
     */
    void pauseAllTarget();

    /**
     * @en
     * Pause all selectors from all targets with a minimum priority. <br/>
     * You should only call this with kCCPriorityNonSystemMin or higher.
     * @zh
     * ???????????????????????????????????????????????????????????????<br/>
     * ??????????????????????????????????????? PRIORITY_NON_SYSTEM_MIN ???????????????
     * @param minPriority
     */
    void pauseAllTargetsWithMinPriority(Priority minPriority);

    /**
     * @en
     * Resume selectors on a set of targets.<br/>
     * This can be useful for undoing a call to pauseAllCallbacks.
     * @zh
     * ????????????????????????????????????????????????<br/>
     * ??????????????? pauseAllCallbacks ???????????????
     * @param targetsToResume
     */
    void resumeTargets(ccSchedulerFunc& targetsToResume);

    /**
     * @en
     * Resumes the target.<br/>
     * The 'target' will be unpaused, so all schedule selectors/update will be 'ticked' again.<br/>
     * If the target is not present, nothing happens.
     * @zh
     * ???????????????????????????????????????<br/>
     * ????????????????????????????????????????????????<br/>
     * ???????????????????????????????????????????????????????????????
     * @param target
     */
    void resumeTarget(ISchedulable* target);

    /**
     * @en Returns whether or not the target is paused.
     * @zh ?????????????????????????????????????????????????????????
     * @param target
     */
    bool isTargetPaused(ISchedulable* target) const;
};

} // namespace cc