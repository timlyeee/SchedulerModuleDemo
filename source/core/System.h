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
#include <string>
namespace cc {
#define _USRDLL
#if (CC_PLATFORM == CC_PLATFORM_WINDOWS)
#if defined(CC_STATIC)
#define CC_DLL
#else
#if defined(_USRDLL)
#define CC_DLL __declspec(dllexport)
#else /* use a DLL library */
#define CC_DLL __declspec(dllimport)
#endif
#endif
#else
#define CC_DLL
#endif

struct ISchedulable {
    std::string id;
    std::string uuid;
};

enum struct Priority : uint32_t {
    LOW       = 0,
    MEDIUM    = 100,
    HIGH      = 200,
    SCHEDULER = (1 << 31),
};

class System : public ISchedulable {
private:
    /* data */
protected:
    Priority _priority{Priority::LOW};
    bool     _executeInEditMode{false};

public:
    /**
     * @en Sorting between different systems.
     * @zh ????????????????????????
     * @param a System a
     * @param b System b
     */
    static int32_t sortByPriority(System* a, System* b) {
        if (a->_priority < b->_priority) {
            return 1;
        } else if (a->_priority > b->_priority) {
            return -1;
        } else {
            return 0;
        }
    }
    
    System() = default;
    virtual ~System() = default;

    inline const std::string& getId() { return id; }
    inline void        setId(std::string& s) { id = s; }

    inline Priority getPriority() const { return _priority; }
    inline void     setPriority(Priority i) { _priority = i; }

    inline bool getExecuteInEditMode() const { return _executeInEditMode; }
    inline void setExecuteInEditMode(bool b) { _executeInEditMode = b; }

    

    /**
     * @en Init the system, will be invoked by [[Director]] when registered, should be implemented if needed.
     * @zh ?????????????????????????????????????????? [[Director]] ????????????????????????????????????????????????
     */
    virtual void init() = 0;

    /**
     * @en Update function of the system, it will be invoked between all components update phase and late update phase.
     * @zh ??????????????????????????????????????????????????? update ??? lateUpdate ???????????????
     * @param dt Delta time after the last frame
     */
    virtual void update(float dt) = 0;

    /**
     * @en Post update function of the system, it will be invoked after all components late update phase and before the rendering process.
     * @zh ?????????????????????????????????????????????????????? lateUpdate ?????????????????????????????????
     * @param dt Delta time after the last frame
     */
    virtual void postUpdate(float dt) = 0;
};
} // namespace cc
