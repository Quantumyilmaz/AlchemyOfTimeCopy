#include "Settings.h"

const bool Settings::IsQFormType(const FormID formid, const std::string& qformtype)
{
    // POPULATE THIS
    if (qformtype == "FOOD") return IsFoodItem(formid);
    else if (qformtype == "INGR") return FormIsOfType(formid, RE::IngredientItem::FORMTYPE);
    else if (qformtype == "MEDC") return IsMedicineItem(formid);
    else if (qformtype == "POSN") return IsPoisonItem(formid);
    else if (qformtype == "ARMO") return FormIsOfType(formid,RE::TESObjectARMO::FORMTYPE);
    else if (qformtype == "WEAP") return FormIsOfType(formid,RE::TESObjectWEAP::FORMTYPE);
    else if (qformtype == "SCRL") return FormIsOfType(formid, RE::ScrollItem::FORMTYPE);
	else if (qformtype == "BOOK") return FormIsOfType(formid,RE::TESObjectBOOK::FORMTYPE);
    else if (qformtype == "SLGM") return FormIsOfType(formid, RE::TESSoulGem::FORMTYPE);
	else if (qformtype == "MISC") return FormIsOfType(formid,RE::TESObjectMISC::FORMTYPE);
    else return false;
}

std::string Settings::GetQFormType(const FormID formid)
{
    for (const auto& q_ftype : QFORMS) {
        if (Settings::IsQFormType(formid,q_ftype)) return q_ftype;
	}
	return "";
}

const bool Settings::IsInExclude(const FormID formid, std::string type)
{
	auto form = GetFormByID(formid);
    if (!form) {
        logger::warn("Form not found.");
        return false;
    }
        
    if (type.empty()) type = GetQFormType(formid);
    if (type.empty()) {
        logger::trace("Type is empty. for formid: {}", formid);
		return false;
	}
    if (!Settings::exclude_list.count(type)) {
		logger::critical("Type not found in exclude list. for formid: {}", formid);
        return false;
    }

    std::string form_string = std::string(form->GetName());
    std::string form_editorid = clib_util::editorID::get_editorID(form);
        
    if (!form_editorid.empty() && String::includesWord(form_editorid, Settings::exclude_list[type])) {
		logger::trace("Form is in exclude list.form_editorid: {}", form_editorid);
		return true;
	}

    /*const auto exlude_list = LoadExcludeList(postfix);*/
    if (String::includesWord(form_string, Settings::exclude_list[type])) {
        logger::trace("Form is in exclude list.form_string: {}", form_string);
        return true;
    }
    return false;
}

const bool Settings::IsItem(const FormID formid, std::string type, bool check_exclude)
{
    if (!formid) return false;
    if (check_exclude && Settings::IsInExclude(formid, type)) return false;
    if (type.empty()) return !GetQFormType(formid).empty();
	else return IsQFormType(formid, type);
}

const bool Settings::IsItem(const RE::TESObjectREFR* ref, std::string type)
{
    if (!ref) return false;
    if (ref->IsDisabled()) return false;
    if (ref->IsDeleted()) return false;
    const auto base = ref->GetBaseObject();
    if (!base) return false;
    return IsItem(base->GetFormID(),type);
}