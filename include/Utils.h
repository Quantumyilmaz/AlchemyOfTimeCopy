#pragma once
#include <windows.h>
#include "ClibUtil/editorID.hpp"

const auto mod_name = static_cast<std::string>(SKSE::PluginDeclaration::GetSingleton()->GetName());
constexpr auto po3path = "Data/SKSE/Plugins/po3_Tweaks.dll";
constexpr auto po3_UoTpath = "Data/SKSE/Plugins/po3_UseOrTake.dll";
inline bool IsPo3Installed() { return std::filesystem::exists(po3path); };
inline bool IsPo3_UoTInstalled() { return std::filesystem::exists(po3_UoTpath); };

const auto po3_err_msgbox = std::format(
        "{}: If you are trying to use Editor IDs, but you must have powerofthree's Tweaks "
        "installed. See mod page for further instructions.",
        mod_name);
const auto general_err_msgbox = std::format("{}: Something went wrong. Please contact the mod author.", mod_name);
const auto init_err_msgbox = std::format("{}: The mod failed to initialize and will be terminated.", mod_name);


inline bool isValidHexWithLength7or8(const char* input);

template <class T = RE::TESForm>
static T* GetFormByID(const RE::FormID id, const std::string& editor_id="") {
    if (!editor_id.empty()) {
        auto* form = RE::TESForm::LookupByEditorID<T>(editor_id);
        if (form) return form;
    }
    T* form = RE::TESForm::LookupByID<T>(id);
    if (form) return form;
    return nullptr;
};

const std::string GetEditorID(const FormID a_formid);

FormID GetFormEditorIDFromString(const std::string formEditorId);

inline bool FormIsOfType(RE::TESForm* form, RE::FormType type);

bool IsFoodItem(RE::TESForm* form);

inline bool IsFoodItem(const FormID formid) { return IsFoodItem(GetFormByID(formid)); }

bool IsPoisonItem(RE::TESForm* form);

inline bool FormIsOfType(const FormID formid, RE::FormType type) { return FormIsOfType(GetFormByID(formid), type); }

inline bool IsPoisonItem(const FormID formid) { return IsPoisonItem(GetFormByID(formid)); }

inline bool IsMedicineItem(RE::TESForm* form);

inline bool IsMedicineItem(const FormID formid) { return IsPoisonItem(GetFormByID(formid)); }

void OverrideMGEFFs(RE::BSTArray<RE::Effect*>& effect_array, std::vector<FormID> new_effects,
                            std::vector<uint32_t> durations, std::vector<float> magnitudes);

inline const bool IsDynamicFormID(const FormID a_formID) { return a_formID >= 0xFF000000; }

void FavoriteItem(RE::TESBoundObject* item, RE::TESObjectREFR* inventory_owner);

[[nodiscard]] const bool IsFavorited(RE::TESBoundObject* item, RE::TESObjectREFR* inventory_owner);

[[nodiscard]] inline const bool IsFavorited(RE::FormID formid, RE::FormID refid) {
    return IsFavorited(GetFormByID<RE::TESBoundObject>(formid),GetFormByID<RE::TESObjectREFR>(refid));
}

inline void FavoriteItem(const FormID formid, const FormID refid) {
	FavoriteItem(GetFormByID<RE::TESBoundObject>(formid), GetFormByID<RE::TESObjectREFR>(refid));
}

[[nodiscard]] inline const bool IsPlayerFavorited(RE::TESBoundObject* item) {
    return IsFavorited(item, RE::PlayerCharacter::GetSingleton()->AsReference());
}

void EquipItem(RE::TESBoundObject* item, bool unequip = false);

inline void EquipItem(const FormID formid, bool unequip = false) {
	EquipItem(GetFormByID<RE::TESBoundObject>(formid), unequip);
}

[[nodiscard]] const bool IsEquipped(RE::TESBoundObject* item);

[[nodiscard]] inline const bool IsEquipped(const FormID formid) {
	return IsEquipped(GetFormByID<RE::TESBoundObject>(formid));
}

namespace Types {

    struct FormFormID {
		FormID form_id1;
		FormID form_id2;

        bool operator<(const FormFormID& other) const {
			// Compare form_id1 first
			if (form_id1 < other.form_id1) {
				return true;
			}
			// If form_id1 is equal, compare form_id2
			if (form_id1 == other.form_id1 && form_id2 < other.form_id2) {
				return true;
			}
			// If both form_id1 and form_id2 are equal or if form_id1 is greater, return false
			return false;
		}
	};

    struct FormEditorID {
        FormID form_id=0;
        std::string editor_id = "";

        bool operator<(const FormEditorID& other) const;
    };

    struct FormEditorIDX : FormEditorID {
        bool is_fake = false;
        bool is_decayed = false;
        bool is_transforming = false;
        bool crafting_allowed = false;


        bool operator==(const FormEditorIDX& other) const;
	};

};

namespace Vector {
    template <typename T>
    inline bool HasElement(const std::vector<T>& vec, const T& element) { return std::find(vec.begin(), vec.end(), element) != vec.end(); };
};

namespace Math {
    namespace LinAlg {
        namespace R3 {
            void rotateX(RE::NiPoint3& v, float angle);

            // Function to rotate a vector around the y-axis
            void rotateY(RE::NiPoint3& v, float angle);

            // Function to rotate a vector around the z-axis
            void rotateZ(RE::NiPoint3& v, float angle);

            void rotate(RE::NiPoint3& v, float angleX, float angleY, float angleZ);
        };
    };
};

namespace String {

    template <typename T>
    std::string join(const T& container, const std::string_view& delimiter) {
        std::ostringstream oss;
        auto iter = container.begin();

        if (iter != container.end()) {
            oss << *iter;
            ++iter;
        }

        for (; iter != container.end(); ++iter) {
            oss << delimiter << *iter;
        }

        return oss.str();
    }

    std::string toLowercase(const std::string& str);

    std::string replaceLineBreaksWithSpace(const std::string& input);

    std::string trim(const std::string& str);

    bool includesWord(const std::string& input, const std::vector<std::string>& strings);
};

namespace MsgBoxesNotifs {
    namespace Windows {

        inline int Po3ErrMsg() {
            MessageBoxA(nullptr, po3_err_msgbox.c_str(), "Error", MB_OK | MB_ICONERROR);
            return 1;
        };
    };

    namespace InGame{
        inline void CustomMsg(const std::string& msg) { RE::DebugMessageBox((mod_name + ": " + msg).c_str()); };
        inline void GeneralErr() { RE::DebugMessageBox(general_err_msgbox.c_str()); };
        inline void InitErr() { RE::DebugMessageBox(init_err_msgbox.c_str()); };
    };
    
};

namespace xData {

    namespace Copy {
        void CopyEnchantment(RE::ExtraEnchantment* from, RE::ExtraEnchantment* to);
            
        void CopyHealth(RE::ExtraHealth* from, RE::ExtraHealth* to);

        void CopyRank(RE::ExtraRank* from, RE::ExtraRank* to);

        void CopyTimeLeft(RE::ExtraTimeLeft* from, RE::ExtraTimeLeft* to);

        void CopyCharge(RE::ExtraCharge* from, RE::ExtraCharge* to);

        void CopyScale(RE::ExtraScale* from, RE::ExtraScale* to);

        void CopyUniqueID(RE::ExtraUniqueID* from, RE::ExtraUniqueID* to);

        void CopyPoison(RE::ExtraPoison* from, RE::ExtraPoison* to);

        void CopyObjectHealth(RE::ExtraObjectHealth* from, RE::ExtraObjectHealth* to);

        void CopyLight(RE::ExtraLight* from, RE::ExtraLight* to);

        void CopyRadius(RE::ExtraRadius* from, RE::ExtraRadius* to);

        void CopyHorse(RE::ExtraHorse* from, RE::ExtraHorse* to);

        void CopyHotkey(RE::ExtraHotkey* from, RE::ExtraHotkey* to);

        void CopyTextDisplayData(RE::ExtraTextDisplayData* from, RE::ExtraTextDisplayData* to);

        void CopySoul(RE::ExtraSoul* from, RE::ExtraSoul* to);

        void CopyOwnership(RE::ExtraOwnership* from, RE::ExtraOwnership* to);
    };

    template <typename T>
    void CopyExtraData(T* from, T* to){
        if (!from || !to) return;
        switch (T->EXTRADATATYPE) {
            case RE::ExtraDataType::kEnchantment:
                CopyEnchantment(from, to);
                break;
            case RE::ExtraDataType::kHealth:
                CopyHealth(from, to);
                break;
            case RE::ExtraDataType::kRank:
                CopyRank(from, to);
                break;
            case RE::ExtraDataType::kTimeLeft:
                CopyTimeLeft(from, to);
                break;
            case RE::ExtraDataType::kCharge:
                CopyCharge(from, to);
                break;
            case RE::ExtraDataType::kScale:
                CopyScale(from, to);
                break;
            case RE::ExtraDataType::kUniqueID:
                CopyUniqueID(from, to);
                break;
            case RE::ExtraDataType::kPoison:
                CopyPoison(from, to);
                break;
            case RE::ExtraDataType::kObjectHealth:
                CopyObjectHealth(from, to);
                break;
            case RE::ExtraDataType::kLight:
                CopyLight(from, to);
                break;
            case RE::ExtraDataType::kRadius:
                CopyRadius(from, to);
                break;
            case RE::ExtraDataType::kHorse:
                CopyHorse(from, to);
				break;
            case RE::ExtraDataType::kHotkey:
                CopyHotkey(from, to);
				break;
            case RE::ExtraDataType::kTextDisplayData:
				CopyTextDisplayData(from, to);
				break;
            case RE::ExtraDataType::kSoul:
				CopySoul(from, to);
                break;
            case RE::ExtraDataType::kOwnership:
                CopyOwnership(from, to);
                break;
            default:
                logger::warn("ExtraData type not found");
                break;
        };
    }

    [[nodiscard]] const bool UpdateExtras(RE::ExtraDataList* copy_from, RE::ExtraDataList* copy_to);
};

namespace WorldObject {

    void SetObjectCount(RE::TESObjectREFR* ref, Count count);

    RE::TESObjectREFR* DropObjectIntoTheWorld(RE::TESBoundObject* obj, Count count, bool owned = true);

    void SwapObjects(RE::TESObjectREFR* a_from, RE::TESBoundObject* a_to, const bool apply_havok=true);

    [[nodiscard]] const bool PlayerPickUpObject(RE::TESObjectREFR* item, Count count, const unsigned int max_try = 3);
};

namespace Inventory {

    const bool EntryHasXData(RE::InventoryEntryData* entry);

    const bool HasItemEntry(RE::TESBoundObject* item, RE::TESObjectREFR* inventory_owner, bool nonzero_entry_check=false);

    inline const std::int32_t GetItemCount(RE::TESBoundObject* item, RE::TESObjectREFR* inventory_owner);
};

namespace DynamicForm {

    void copyBookAppearence(RE::TESForm* source, RE::TESForm* target);

    template <class T>
    static void copyComponent(RE::TESForm* from, RE::TESForm* to) {
        auto fromT = from->As<T>();

        auto toT = to->As<T>();

        if (fromT && toT) {
            toT->CopyComponent(fromT);
        }
    }

    void copyFormArmorModel(RE::TESForm* source, RE::TESForm* target);

    void copyFormObjectWeaponModel(RE::TESForm* source, RE::TESForm* target);

    void copyMagicEffect(RE::TESForm* source, RE::TESForm* target);

    void copyAppearence(RE::TESForm* source, RE::TESForm* target);

};


template <typename T>
struct FormTraits {
    static float GetWeight(T* form) {
        // Default implementation, assuming T has a member variable 'weight'
        return form->weight;
    }

    static void SetWeight(T* form, float weight) {
        // Default implementation, set the weight if T has a member variable 'weight'
        form->weight = weight;
    }
    
    static int GetValue(T* form) {
		// Default implementation, assuming T has a member variable 'value'
		return form->value;
	}

    static void SetValue(T* form, int value) {
        form->value = value;
    }

    static RE::BSTArray<RE::Effect*> GetEffects(T*) { 
        RE::BSTArray<RE::Effect*> effects;
        return effects;
	}
};

template <>
struct FormTraits<RE::AlchemyItem> {
    static float GetWeight(RE::AlchemyItem* form) { 
        return form->weight;
    }

    static void SetWeight(RE::AlchemyItem* form, float weight) { 
        form->weight = weight;
    }

    static int GetValue(RE::AlchemyItem* form) {
        return form->GetGoldValue();
    }
    static void SetValue(RE::AlchemyItem* form, int value) { 
        logger::trace("CostOverride: {}", form->data.costOverride);
        form->data.costOverride = value;
    }

    static RE::BSTArray<RE::Effect*> GetEffects(RE::AlchemyItem* form) {
		return form->effects;
	}
};

template <>
struct FormTraits<RE::IngredientItem> {
	static float GetWeight(RE::IngredientItem* form) { 
		return form->weight;
	}

	static void SetWeight(RE::IngredientItem* form, float weight) { 
		form->weight = weight;
	}

	static int GetValue(RE::IngredientItem* form) {
		return form->GetGoldValue();
	}
	static void SetValue(RE::IngredientItem* form, int value) { 
		form->value = value;
	}

	static RE::BSTArray<RE::Effect*> GetEffects(RE::IngredientItem* form) {
        return form->effects;
    }
};
        
template <>
struct FormTraits<RE::TESAmmo> {
    static float GetWeight(RE::TESAmmo* form) {
        // Default implementation, assuming T has a member variable 'weight'
        return form->GetWeight();
    }

    static void SetWeight(RE::TESAmmo*, float) {
        // Default implementation, set the weight if T has a member variable 'weight'
        return;
    }

    static int GetValue(RE::TESAmmo* form) {
        // Default implementation, assuming T has a member variable 'value'
        return form->value;
    }

    static void SetValue(RE::TESAmmo* form, int value) { form->value = value; }

    static RE::BSTArray<RE::Effect*> GetEffects(RE::TESAmmo*) {
        RE::BSTArray<RE::Effect*> effects;
        return effects;
    }
};