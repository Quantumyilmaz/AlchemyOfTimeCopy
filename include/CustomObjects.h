#pragma once
#include "Utils.h"

using Duration = float;
using DurationMGEFF = std::uint32_t;
using StageNo = unsigned int;
using StageName = std::string;

struct StageEffect {
    FormID beffect;          // base effect
    float magnitude;         // in effectitem
    std::uint32_t duration;  // in effectitem (not Duration, this is in seconds)

    StageEffect() : beffect(0), magnitude(0), duration(0) {}
    StageEffect(FormID be, float mag, DurationMGEFF dur) : beffect(be), magnitude(mag), duration(dur) {}

    [[nodiscard]] bool IsNull() const { return beffect == 0; }
    [[nodiscard]] bool HasMagnitude() const { return magnitude != 0; }
    [[nodiscard]] bool HasDuration() const { return duration != 0; }
};


struct Stage {
    FormID formid = 0;  // with which item is it represented
    Duration duration;  // duration of the stage
    StageNo no;         // used for sorting when multiple stages are present
    StageName name;     // name of the stage
    std::vector<StageEffect> mgeffect;

    bool crafting_allowed;


    Stage(){};
    Stage(FormID f, Duration d, StageNo s, StageName n, bool ca,std::vector<StageEffect> e)
        : formid(f), duration(d), no(s), name(n), crafting_allowed(ca) ,mgeffect(e) {
        if (!formid) logger::critical("FormID is null");
        else logger::trace("Stage: FormID {}, Duration {}, StageNo {}, Name {}", formid, duration, no, name);
        if (e.empty()) mgeffect.clear();
        if (duration <= 0) {
			logger::critical("Duration is 0 or negative");
			duration = 0.1f;
        }
    }

    bool operator<(const Stage& other) const {
        if (formid < other.formid) return true;
        if (other.formid < formid) return false;
        return no < other.no;
    }

    bool operator==(const Stage& other) const {
        return no == other.no && formid == other.formid && duration == other.duration;
    }

    RE::TESBoundObject* GetBound() const { return GetFormByID<RE::TESBoundObject>(formid); };

    [[nodiscard]] bool CheckIntegrity() const;

    inline const char* GetExtraText() const { return GetBound()->GetName(); }

};

struct StageInstancePlain{
    float start_time;
    StageNo no;
    Count count;
            
    float _elapsed;
    float _delay_start;
    float _delay_mag;
    FormID _delay_formid;

    bool is_fake = false;
    bool is_decayed = false;
    bool is_transforming = false;

    bool is_faved = false;
    bool is_equipped = false;

    FormID form_id=0; // for fake stuff
};

struct StageInstance {
    float start_time; // start time of the stage
    StageNo no;
    Count count;
    //RefID location;  // RefID of the container where the fake food is stored or the real food itself when it is
                        // out in the world
    Types::FormEditorIDX xtra;

    //StageInstance() : start_time(0), no(0), count(0), location(0) {}
    StageInstance(const float st, const StageNo n, const Count c)
        : start_time(st), no(n), count(c)
    {
        _elapsed = 0;
        _delay_start = start_time;
        _delay_mag = 1;
        _delay_formid = 0;
    }
        
    //define ==
    // assumes that they are in the same inventory
	[[nodiscard]] bool operator==(const StageInstance& other) const;

    // times are very close (abs diff less than 0.015h = 0.9min)
    // assumes that they are in the same inventory
	[[nodiscard]] bool AlmostSameExceptCount(StageInstance& other, float curr_time) const;

    StageInstance& operator=(const StageInstance& other);

    inline RE::TESBoundObject* GetBound() const { return GetFormByID<RE::TESBoundObject>(xtra.form_id); };

    float GetElapsed(float curr_time) const;

    float GetDelaySlope() const;

    void SetNewStart(float curr_time, float overshot);

    void SetDelay(float time, float delay, FormID formid);

    void SetTransform(float time, FormID formid);

    float GetTransformElapsed(const float curr_time) const { return GetElapsed(curr_time) - _elapsed; }

    void RemoveTransform(float curr_time);

    // use only for WO (e.g. HandleDrop)
    void RemoveTimeMod(float time);

    float GetDelayMagnitude() const { return GetDelaySlope(); }

    FormID GetDelayerFormID() const { return _delay_formid; }

    float GetHittingTime(const float schranke) const {
        // _elapsed + dt*_delay_mag = schranke
        return _delay_start + (schranke - _elapsed) / (GetDelaySlope() + std::numeric_limits<float>::epsilon());
    };

    float GetTransformHittingTime(const float schranke) const{
        if (!xtra.is_transforming) return 0;
		return GetHittingTime(schranke+_elapsed);
    }

    StageInstancePlain GetPlain() const;

    void SetDelay(const StageInstancePlain& plain);

private:
    float _elapsed; // y coord of the ausgangspunkt/elapsed time since the stage started
    float _delay_start;  // x coord of the ausgangspunkt
    float _delay_mag; // slope
    FormID _delay_formid; // formid of the time modulator

};

struct StageUpdate {
    const Stage* oldstage;
    const Stage* newstage;
    Count count=0;
    Duration update_time=0;
    bool new_is_fake=false;

    StageUpdate(const Stage* old, const Stage* new_, Count c, 
        Duration u_t,
        bool fake)
		: oldstage(old), newstage(new_), count(c), 
        update_time(u_t), 
        new_is_fake(fake) {}
};

struct DefaultSettings {
    std::map<StageNo, FormID> items = {};
    std::map<StageNo, Duration> durations = {};
    std::map<StageNo, StageName> stage_names = {};
    std::map<StageNo, bool> crafting_allowed = {};
    std::map<StageNo, int> costoverrides = {};
    std::map<StageNo, float> weightoverrides = {};
    std::map<StageNo, std::vector<StageEffect>> effects = {};
    std::vector<StageNo> numbers = {};
    FormID decayed_id = 0;

    std::map<FormID,float> delayers;
    std::map<FormID, std::tuple<FormID, Duration, std::vector<StageNo>>> transformers;

    [[nodiscard]] bool IsHealthy() const { return !init_failed; }

    [[nodiscard]] bool CheckIntegrity();

    [[nodiscard]] bool IsEmpty();

private:
    bool init_failed = false;
};

using CustomSettings = std::map<std::vector<std::string>, DefaultSettings>;