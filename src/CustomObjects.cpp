#include "CustomObjects.h"

bool StageInstance::operator==(const StageInstance& other) const
{
    return no == other.no && count == other.count && 
        //location == other.location &&
            start_time == other.start_time && 
        _elapsed == other._elapsed && xtra == other.xtra;
}

bool StageInstance::AlmostSameExceptCount(StageInstance& other, const float curr_time) const
{
    // bcs they are in the same inventory they will have same delay magnitude
            // delay starts might be different but if the elapsed times are close enough, we don't care
    return no == other.no && 
        //location == other.location &&
            std::abs(start_time - other.start_time) < 0.015 &&
            std::abs(GetElapsed(curr_time) - other.GetElapsed(curr_time)) < 0.015 && xtra == other.xtra;
}

StageInstance& StageInstance::operator=(const StageInstance& other)
{
    if (this != &other) {
                    
        start_time = other.start_time;
        no = other.no;
        count = other.count;
        //location = other.location;
                    
        xtra = other.xtra;

        _elapsed = other._elapsed;
        _delay_start = other._delay_start;
        _delay_mag = other._delay_mag;
        _delay_formid = other._delay_formid;
    }
    return *this;
}

float StageInstance::GetElapsed(const float curr_time) const {
    if (fabs(_delay_mag) < EPSILON) return _elapsed;
    return (curr_time - _delay_start) * GetDelaySlope() + _elapsed;
}

float StageInstance::GetDelaySlope() const {
    return std::min(std::max(-1000.f, _delay_mag), 1000.f);
}

void StageInstance::SetNewStart(const float curr_time, const float overshot)
{
    // overshot: by how much is the schwelle already ueberschritten
    start_time = curr_time - overshot / (GetDelaySlope() + std::numeric_limits<float>::epsilon());
    _delay_start = start_time;
    _elapsed = 0;
}

void StageInstance::SetDelay(const float time, const float delay, const FormID formid)
{
    // yeni steigungla yeni ausgangspunkt yapiyoruz
    // call only from UpdateTimeModulationInInventory
    if (xtra.is_transforming) return;
    if (fabs(_delay_mag - delay) < EPSILON && _delay_formid == formid) return;

    _elapsed = GetElapsed(time);
    _delay_start = time;
	_delay_mag = delay;
    _delay_formid = formid;
}

void StageInstance::SetTransform(const float time, const FormID formid)
{
    if (xtra.is_transforming){
        if (_delay_formid != formid) {
            RemoveTransform(time);
            return SetTransform(time, formid);
        }
        return;
    } 
    SetDelay(time, 1, formid);
    xtra.is_transforming = true;
}

void StageInstance::RemoveTransform(const float curr_time)
{
    if (!xtra.is_transforming) return;
    xtra.is_transforming = false;
    _delay_start = curr_time;
    _delay_mag = 1;
    _delay_formid = 0;
}

void StageInstance::RemoveTimeMod(const float time)
{
    RemoveTransform(time);
    SetDelay(time, 1, 0);
}

StageInstancePlain StageInstance::GetPlain() const
{
    StageInstancePlain plain;
    plain.start_time = start_time;
    plain.no = no;
    plain.count = count;

    plain.is_fake = xtra.is_fake;
    plain.is_decayed = xtra.is_decayed;
    plain.is_transforming = xtra.is_transforming;

    plain._elapsed = _elapsed;
    plain._delay_start = _delay_start;
    plain._delay_mag = _delay_mag;
    plain._delay_formid = _delay_formid;

    if (xtra.is_fake) plain.form_id = xtra.form_id;
                
    return plain;
}

void StageInstance::SetDelay(const StageInstancePlain& plain)
{
    _elapsed = plain._elapsed;
	_delay_start = plain._delay_start;
	_delay_mag = plain._delay_mag;
	_delay_formid = plain._delay_formid;
}

bool Stage::CheckIntegrity() const {
    if (!formid || !GetBound()) {
		logger::error("FormID or bound is null");
		return false;
	}
    if (duration <= 0) {
        logger::error("Duration is 0 or negative");
        return false;
    }
	return true;
}


bool DefaultSettings::CheckIntegrity() {
    if (items.empty() || durations.empty() || stage_names.empty() || effects.empty() || numbers.empty()) {
        logger::error("One of the maps is empty.");
        logger::trace("Items size: {}, Durations size: {}, Stage names size: {}, Effects size: {}, Numbers size: {}",
                      items.size(), durations.size(), stage_names.size(), effects.size(), numbers.size());
        init_failed = true;
        return false;
    }
    if (items.size() != durations.size() || items.size() != stage_names.size() || items.size() != numbers.size()) {
		logger::error("Sizes do not match.");
        init_failed = true;
		return false;
	}
    for (auto i = 0; i < numbers.size(); i++) {
        if (!Vector::HasElement<StageNo>(numbers, i)) {
            logger::error("Key {} not found in numbers.", i);
            return false;
        }
        if (!items.contains(i) || !crafting_allowed.contains(i) || !durations.contains(i) || !stage_names.contains(i) ||
            !effects.contains(i)) {
			logger::error("Key {} not found in all maps.", i);
            init_failed = true;
			return false;
		}
				
        if (durations[i] <= 0) {
			logger::error("Duration is less than or equal 0.");
			init_failed = true;
			return false;
		}
                
        if (!costoverrides.contains(i)) costoverrides[i] = -1;
		if (!weightoverrides.contains(i)) weightoverrides[i] = -1.0f;

	}
    if (!decayed_id) {
        logger::error("Decayed id is 0.");
        init_failed = true;
        return false;
    }
    for (const auto& [_formID, _transformer] : transformers) {
        const FormID _finalFormEditorID = std::get<0>(_transformer);
        const Duration _duration = std::get<1>(_transformer);
        std::vector<StageNo> _allowedStages = std::get<2>(_transformer);
        if (!GetFormByID(_formID) || !GetFormByID(_finalFormEditorID)) {
			logger::error("Formid not found.");
			init_failed = true;
			return false;
		}
        if (_duration <= 0) {
			logger::error("Duration is less than or equal 0.");
			init_failed = true;
			return false;
		}
		if (_allowedStages.empty()) {
			logger::error("Allowed stages is empty.");
			init_failed = true;
			return false;
		}
		for (const auto& _stage : _allowedStages) {
			if (!Vector::HasElement<StageNo>(numbers, _stage)) {
				logger::error("Stage {} not found in numbers.", _stage);
				init_failed = true;
				return false;
			}
		}

	}

	for (const auto& _formID : delayers | std::views::keys) {
		if (!GetFormByID(_formID)) {
			logger::error("Delayer formid {} not found.", _formID);
			init_failed = true;
			return false;
		}
	}

    for (const auto& _formID : containers) {
		if (!GetFormByID(_formID)) {
			logger::error("Container formid {} not found.", _formID);
			init_failed = true;
			return false;
		}
	}
	return true;
}

bool DefaultSettings::IsEmpty()
{
	if (numbers.empty()) {
		init_failed = true;
	    return true;
	}
	return false;
}

void DefaultSettings::Add(const AddOnSettings& addon)
{
    if (addon.transformers.empty()) {
		logger::error("Transformers is empty.");
    }
	for (const auto& _formID : addon.containers) {
        if (!_formID) {
            logger::critical("AddOn has null formid.");
	        continue;
        }
		containers.insert(_formID);
    }
	for (const auto& [_formID, _delay] : addon.delayers) {
        if (!_formID) {
            logger::critical("AddOn has null formid.");
	        continue;
        }
        if (!delayers.contains(_formID)) {
			delayers_order.push_back(_formID);
		}
		delayers[_formID] = _delay;
    }
	for (const auto& [_formID, _transformer] : addon.transformers) {
        if (!_formID) {
            logger::critical("AddOn has null formid.");
	        continue;
        }
		if (!transformers.contains(_formID)) {
			transformers_order.push_back(_formID);
        }
		transformers[_formID] = _transformer;
    }
	for (const auto& [_formID, _color] : addon.delayer_colors) {
        if (!_formID) {
            logger::critical("AddOn has null formid.");
	        continue;
        }
		delayer_colors[_formID] = _color;
    }
	for (const auto& [_formID, _color] : addon.transformer_colors) {
        if (!_formID) {
            logger::critical("AddOn has null formid.");
	        continue;
        }
		transformer_colors[_formID] = _color;
    }
}

bool AddOnSettings::CheckIntegrity()
{
    for (const auto& [_formID, _transformer] : transformers) {
        const FormID _finalFormEditorID = std::get<0>(_transformer);
        const Duration _duration = std::get<1>(_transformer);
        std::vector<StageNo> _allowedStages = std::get<2>(_transformer);
        if (!GetFormByID(_formID) || !GetFormByID(_finalFormEditorID)) {
			logger::error("Form not found.");
			init_failed = true;
			return false;
		}
        if (_duration <= 0) {
			logger::error("Duration is less than or equal 0.");
			init_failed = true;
			return false;
		}
	}

	for (const auto& _formID : delayers | std::views::keys) {
		if (!GetFormByID(_formID)) {
			logger::error("Delayer form {} not found.", _formID);
			init_failed = true;
			return false;
		}
	}
	for (const auto& _formID : containers) {
		if (!GetFormByID(_formID)) {
			logger::error("Container form {} not found.", _formID);
			init_failed = true;
			return false;
		}
	}

	return true;
}