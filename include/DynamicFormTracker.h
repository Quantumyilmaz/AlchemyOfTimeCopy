﻿#pragma once
#include "Serialization.h"

struct ActEff {
    FormID baseFormid;
    FormID dynamicFormid;
    float elapsed;
    std::pair<bool, uint32_t> custom_id;

};

class DynamicFormTracker : public DFSaveLoadData {
    
    // created form bank during the session. Create populates this.
    std::map<std::pair<FormID, std::string>, std::set<FormID>> forms;
    std::map<FormID, uint32_t> customIDforms; // Fetch populates this

    std::set<FormID> active_forms; // _yield populates this
    std::set<FormID> protected_forms;
    std::set<FormID> deleted_forms;


	std::shared_mutex forms_mutex;
	std::shared_mutex customIDforms_mutex;
    std::shared_mutex active_forms_mutex;
	std::shared_mutex protected_forms_mutex;
	std::shared_mutex deleted_forms_mutex;
	std::shared_mutex act_effs_mutex;

    bool block_create = false;

    //std::map<FormID,float> act_effs;
    std::vector<ActEff> act_effs; // save file specific

    void CleanseFormsets() {
        for (auto it = forms.begin(); it != forms.end();++it) {
            auto& [base, formset] = *it;
            for (auto it2 = formset.begin(); it2 != formset.end();) {
                const auto base_form = RE::TESForm::LookupByID(base.first);
                const auto newForm = RE::TESForm::LookupByID(*it2);
		        const auto refForm = RE::TESForm::LookupByID<RE::TESObjectREFR>(*it2);
                if (!newForm || !underlying_check(base_form, newForm) || refForm) {
                    logger::trace("Form with ID {:x} does not exist. Removing from formset.", *it2);
					std::unique_lock lock(forms_mutex);
					std::unique_lock lock2(customIDforms_mutex);
					std::unique_lock lock3(active_forms_mutex);
                    customIDforms.erase(*it2);
                    active_forms.erase(*it2);
                    Unreserve(*it2);
                    it2 = formset.erase(it2);
                    //deleted_forms.erase(*it2);
                } else {
                    ++it2;
                }
            }
        }
    }

	[[nodiscard]] float GetActiveEffectElapsed(const FormID dyn_formid) {
		std::shared_lock lock(act_effs_mutex);
		for (const auto& act_eff : act_effs) {
			if (act_eff.dynamicFormid == dyn_formid) {
				return act_eff.elapsed;
			}
		}
		return -1.f;
	}

    [[nodiscard]] bool IsTracked(const FormID dynamic_formid) {
		std::shared_lock lock(forms_mutex);
        for (const auto& dyn_formset : forms | std::views::values) {
			if (dyn_formset.contains(dynamic_formid)) {
				return true;
			}
		}
		return false;
    }

    [[maybe_unused]] RE::TESForm* GetOGFormOfDynamic(const FormID dynamic_formid) {
		std::shared_lock lock(forms_mutex);
		for (const auto& [base_pair, dyn_formset] : forms) {
			if (dyn_formset.contains(dynamic_formid)) {
				return GetFormByID(base_pair.first, base_pair.second);
			}
		}
		return nullptr;
	}

    static void ReviveDynamicForm(RE::TESForm* fake, RE::TESForm* base, const FormID setFormID=0) {
        using namespace DynamicForm;
        fake->Copy(base);
        const auto weaponBaseForm = base->As<RE::TESObjectWEAP>();

        const auto weaponNewForm = fake->As<RE::TESObjectWEAP>();

        const auto bookBaseForm = base->As<RE::TESObjectBOOK>();

        const auto bookNewForm = fake->As<RE::TESObjectBOOK>();

        const auto ammoBaseForm = base->As<RE::TESAmmo>();

        const auto ammoNewForm = fake->As<RE::TESAmmo>();

        if (weaponNewForm && weaponBaseForm) {
            weaponNewForm->firstPersonModelObject = weaponBaseForm->firstPersonModelObject;

            weaponNewForm->weaponData = weaponBaseForm->weaponData;

            weaponNewForm->criticalData = weaponBaseForm->criticalData;

            weaponNewForm->attackSound = weaponBaseForm->attackSound;

            weaponNewForm->attackSound2D = weaponBaseForm->attackSound2D;

            weaponNewForm->attackSound = weaponBaseForm->attackSound;

            weaponNewForm->attackFailSound = weaponBaseForm->attackFailSound;

            weaponNewForm->idleSound = weaponBaseForm->idleSound;

            weaponNewForm->equipSound = weaponBaseForm->equipSound;

            weaponNewForm->unequipSound = weaponBaseForm->unequipSound;

            weaponNewForm->soundLevel = weaponBaseForm->soundLevel;

            weaponNewForm->impactDataSet = weaponBaseForm->impactDataSet;

            weaponNewForm->templateWeapon = weaponBaseForm->templateWeapon;

            weaponNewForm->embeddedNode = weaponBaseForm->embeddedNode;

        } else if (bookBaseForm && bookNewForm) {
            bookNewForm->data.flags = bookBaseForm->data.flags;

            bookNewForm->data.teaches.spell = bookBaseForm->data.teaches.spell;

            bookNewForm->data.teaches.actorValueToAdvance = bookBaseForm->data.teaches.actorValueToAdvance;

            bookNewForm->data.type = bookBaseForm->data.type;

            bookNewForm->inventoryModel = bookBaseForm->inventoryModel;

            bookNewForm->itemCardDescription = bookBaseForm->itemCardDescription;

        } else if (ammoBaseForm && ammoNewForm) {
            ammoNewForm->GetRuntimeData().data.damage = ammoBaseForm->GetRuntimeData().data.damage;

            ammoNewForm->GetRuntimeData().data.flags = ammoBaseForm->GetRuntimeData().data.flags;

            ammoNewForm->GetRuntimeData().data.projectile = ammoBaseForm->GetRuntimeData().data.projectile;
        }
        /*else {
            new_form->Copy(baseForm);
        }*/

        copyComponent<RE::TESDescription>(base, fake);

        copyComponent<RE::BGSKeywordForm>(base, fake);

        copyComponent<RE::BGSPickupPutdownSounds>(base, fake);

        copyComponent<RE::TESModelTextureSwap>(base, fake);

        copyComponent<RE::TESModel>(base, fake);

        copyComponent<RE::BGSMessageIcon>(base, fake);

        copyComponent<RE::TESIcon>(base, fake);

        copyComponent<RE::TESFullName>(base, fake);

        copyComponent<RE::TESValueForm>(base, fake);

        copyComponent<RE::TESWeightForm>(base, fake);

        copyComponent<RE::BGSDestructibleObjectForm>(base, fake);

        copyComponent<RE::TESEnchantableForm>(base, fake);

        copyComponent<RE::BGSBlockBashData>(base, fake);

        copyComponent<RE::BGSEquipType>(base, fake);

        copyComponent<RE::TESAttackDamageForm>(base, fake);

        copyComponent<RE::TESBipedModelForm>(base, fake);

        if (setFormID != 0) fake->SetFormID(setFormID, false);
    }

    template <typename T>
    FormID Create(T* baseForm, const RE::FormID setFormID = 0) {
        if (block_create) return 0;

        if (!baseForm) {
            logger::error("Real form is null for baseForm.");
            return 0;
        }

        const auto base_formid = baseForm->GetFormID();
        const auto base_editorid = clib_util::editorID::get_editorID(baseForm);

        if (base_editorid.empty()) {
			logger::error("Failed to get editorID for baseForm.");
			return 0;
		}

        RE::TESForm* new_form = nullptr;

        auto factory = RE::IFormFactory::GetFormFactoryByType(baseForm->GetFormType());

        new_form = factory->Create();

        // new_form = baseForm->CreateDuplicateForm(true, (void*)new_form)->As<T>();

        if (!new_form) {
            logger::error("Failed to create new form.");
            return 0;
        }
        logger::trace("Original form id: {:x}", new_form->GetFormID());

        if (forms[{base_formid, base_editorid}].contains(setFormID)) {
        	logger::warn("Form with ID {:x} already exist for baseid {} and editorid {}.", setFormID, base_formid, base_editorid);
            ReviveDynamicForm(new_form, baseForm);
        } else ReviveDynamicForm(new_form, baseForm, setFormID);

        const auto new_formid = new_form->GetFormID();

        logger::trace("Created form with type: {}, Base ID: {:x}, Name: {}",
                      RE::FormTypeToString(new_form->GetFormType()), new_form->GetFormID(),new_form->GetName());

        if (auto lock = std::unique_lock(forms_mutex); !forms[{base_formid, base_editorid}].insert(new_formid).second) {
            lock.unlock();
            logger::error("Failed to insert new form into forms.");
            if (!_delete({base_formid, base_editorid}, new_formid) && !deleted_forms.contains(new_formid)) {
                logger::critical("Failed to delete form with ID {:x}.", new_formid);
            }
            return 0;
        }

        if (new_formid >= 0xFF3DFFFF){
            logger::critical("Dynamic FormID limit reached!!!!!!");
            block_create = true;
            if (!_delete({base_formid, base_editorid}, new_formid) && !deleted_forms.contains(new_formid)) {
                logger::critical("Failed to delete form with ID {:x}.", new_formid);
            }
			return 0;
        }

        return new_formid;
    }

    FormID GetByCustomID(const uint32_t custom_id, const FormID base_formid, const std::string& base_editorid) {
        const auto formset = GetFormSet(base_formid, base_editorid);
        if (formset.empty()) return 0;
		std::shared_lock lock(customIDforms_mutex);
        for (const auto _formid : formset) {
            if (customIDforms.contains(_formid) && customIDforms.at(_formid) == custom_id) return _formid;
		}
        return 0;
    }

    // makes it active
    const RE::TESForm* _yield(const FormID dynamic_formid, RE::TESForm* base_form) {
        if (const auto newForm = RE::TESForm::LookupByID(dynamic_formid)) {
			if (!underlying_check(base_form, newForm)) {
				logger::error("Underlying check failed for form with ID {:x}.", dynamic_formid);
				return nullptr;
			}
            if (std::strlen(newForm->GetName()) == 0) {
                ReviveDynamicForm(newForm, base_form);
			}
            if (auto lock = std::unique_lock(active_forms_mutex); active_forms.insert(dynamic_formid).second) {
                lock.unlock();
                Unreserve(dynamic_formid);
                if (auto lock2 = std::shared_lock(active_forms_mutex); active_forms.size()>form_limit) {
					logger::warn("Active dynamic forms limit reached!!!");
                    block_create = true;
				}
            }
            
			return newForm;
		}
		return nullptr;
	}

    bool _delete(const std::pair<FormID, std::string>& base, const FormID dynamic_formid) {
        if (auto lock = std::shared_lock(protected_forms_mutex); protected_forms.contains(dynamic_formid)) {
			logger::warn("Form with ID {:x} is protected.", dynamic_formid);
			return false;
		}
        if (auto lock = std::shared_lock(forms_mutex); !forms.contains(base)) return false;

        const auto base_form = RE::TESForm::LookupByID(base.first);
        const auto newForm = RE::TESForm::LookupByID(dynamic_formid);
		const auto refForm = RE::TESForm::LookupByID<RE::TESObjectREFR>(dynamic_formid);

        if (newForm && underlying_check(base_form, newForm) && !refForm) {

            if (const auto bound_temp = newForm->As<RE::TESBoundObject>(); bound_temp) {
                const auto player = RE::PlayerCharacter::GetSingleton();
                auto player_inventory = player->GetInventory();
                if (const auto it = player_inventory.find(bound_temp); it != player_inventory.end()) {
                    player->RemoveItem(bound_temp, it->second.first, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
                }
			}

            //if (auto* virtualMachine = RE::BSScript::Internal::VirtualMachine::GetSingleton()) {
            //    auto* handlePolicy = virtualMachine->GetObjectHandlePolicy();
            //    auto* bindPolicy = virtualMachine->GetObjectBindPolicy();

            //    if (handlePolicy && bindPolicy) {
            //        auto newHandler = handlePolicy->GetHandleForObject(newForm->GetFormType(), newForm);

            //        if (newHandler != handlePolicy->EmptyHandle()) {
            //            auto* vm_scripts_hashmap = &virtualMachine->attachedScripts;
            //            auto newHandlerScripts_it = vm_scripts_hashmap->find(newHandler);

            //            if (newHandlerScripts_it != vm_scripts_hashmap->end()) {
            //                vm_scripts_hashmap[newHandler].clear();
            //            }
            //        }
            //    }
            //}
            logger::warn("Deleting form with ID: {:x}", dynamic_formid);
            delete newForm;
			std::unique_lock lock(deleted_forms_mutex);
            deleted_forms.insert(dynamic_formid);
        }
		std::unique_lock lock(forms_mutex);
		std::unique_lock lock2(customIDforms_mutex);
		std::unique_lock lock3(active_forms_mutex);
        forms[base].erase(dynamic_formid);
        customIDforms.erase(dynamic_formid);
        active_forms.erase(dynamic_formid);
        return true;
    }

    [[nodiscard]] static bool underlying_check(const RE::TESForm* underlying, const RE::TESForm* derivative) {
        if (underlying->GetFormType() != derivative->GetFormType()) {
			logger::trace("Form types do not match: {} vs {}, ID: {:x} vs {:x}",
				RE::FormTypeToString(underlying->GetFormType()), RE::FormTypeToString(derivative->GetFormType()),
				underlying->GetFormID(), derivative->GetFormID());
			logger::trace("Underlying name: {}, Derivative name: {}", underlying->GetName(), derivative->GetName());
            return false;
        }

        // alchemy
        const auto alch_underlying = underlying->As<RE::AlchemyItem>();
        const auto alch_derivative = derivative->As<RE::AlchemyItem>();
        bool asd1 = alch_underlying != nullptr;
        bool asd2 = alch_derivative != nullptr;
        if (const int asd = asd1 + asd2; asd % 2 != 0) {
            logger::trace("Alchemy status does not match.");
            return false;
        }

        if (alch_underlying && alch_derivative) {
            if (alch_underlying->IsPoison() != alch_derivative->IsPoison()) {
                logger::trace("Poison status does not match.");
                return false;
            }

            if (alch_underlying->IsFood() != alch_derivative->IsFood()) {
                logger::trace("Food status does not match.");
                return false;
            }

            if (alch_underlying->IsMedicine() != alch_derivative->IsMedicine()) {
                logger::trace("Medicine status does not match.");
                return false;
            }
        }

        // ingredient
        const auto ingr_underlying = underlying->As<RE::IngredientItem>();
        const auto ingr_derivative = derivative->As<RE::IngredientItem>();
        asd1 = ingr_underlying != nullptr;
        asd2 = ingr_derivative != nullptr;
        if (const int asd = asd1 + asd2; asd % 2 != 0) {
            logger::trace("Ingredient status does not match.");
            return false;
        }

        if (ingr_underlying && ingr_derivative) {
            if (ingr_underlying->IsFood() != ingr_derivative->IsFood()) {
                logger::trace("Food status does not match.");
                return false;
            }

            if (ingr_underlying->IsPoison() != ingr_derivative->IsPoison()) {
                logger::trace("Poison status does not match.");
                return false;
            }

            if (ingr_underlying->IsMedicine() != ingr_derivative->IsMedicine()) {
                logger::trace("Medicine status does not match.");
                return false;
            }
        }

        // POPULATE THIS as you enable other modules

        return true;
    }

public:
    static DynamicFormTracker* GetSingleton() {
        static DynamicFormTracker singleton;
        return &singleton;
    }

    const unsigned int form_limit = 10000;

    const char* GetType() override { return "DynamicFormTracker"; }

    bool IsActive(const FormID a_formid) {
		std::shared_lock lock(active_forms_mutex);
        return active_forms.contains(a_formid);
	}

	bool IsProtected(const FormID a_formid) {
		std::shared_lock lock(protected_forms_mutex);
		return protected_forms.contains(a_formid);
    }

    std::set<FormID> GetFormSet(const FormID base_formid, std::string base_editorid = "") {
        if (base_editorid.empty()) {
            base_editorid = GetEditorID(base_formid);
            if (base_editorid.empty()) {
                return {};
            }
        }
        const std::pair key = {base_formid, base_editorid};
        if (auto lock = std::shared_lock(forms_mutex); forms.contains(key)) return forms.at(key);
        return {};
    }

    void DeleteInactives() {
        logger::trace("Deleting inactives.");
		std::shared_lock lock(forms_mutex);
        for (auto& [base, formset] : forms) {
		    size_t index = 0;
            while (index < formset.size()) {
                auto it = formset.begin();
                std::advance(it, index);
                if (!IsActive(*it) && !IsProtected(*it)) {
				    lock.unlock();
                    if (!_delete(base, *it)) {
                        index++;
					}
					lock.lock();
                }
                else index++;
			}
		}
	}

    std::vector<std::pair<FormID, std::string>> GetSourceForms(){
        std::set<std::pair<FormID, std::string>> source_forms;
		std::shared_lock lock(forms_mutex);
		for (const auto& base : forms | std::views::keys) {
			source_forms.insert(base);
		}
		lock.unlock();
		std::shared_lock lock2(act_effs_mutex);
        for (const auto& [base_formid, dynamicFormid, elapsed, custom_id] : act_effs) {
            const auto base_form = GetFormByID(base_formid);
            if (!base_form) {
				logger::error("Failed to get base form.");
				continue;
			}
            const auto base_editorid = clib_util::editorID::get_editorID(base_form);
            source_forms.insert({base_formid, base_editorid});
		}
		lock2.unlock();

        auto source_forms_vector = std::vector(source_forms.begin(), source_forms.end());

		return source_forms_vector;
    }

    std::vector<FormID> GetDynamicForms() {
		std::vector<FormID> dynamic_forms;
		std::shared_lock lock(forms_mutex);
		for (const auto& formset : forms | std::views::values) {
			for (const auto formid : formset) {
				dynamic_forms.push_back(formid);
			}
		}
		return dynamic_forms;
    }

    void EditCustomID(const FormID dynamic_formid, const uint32_t custom_id) {
		std::unique_lock lock(customIDforms_mutex);
        if (customIDforms.contains(dynamic_formid)) customIDforms[dynamic_formid] = custom_id;
        else if (IsTracked(dynamic_formid)) customIDforms.insert({dynamic_formid, custom_id});
	}

    // tries to fetch by custom id. regardless, returns formid if there is in the bank
    FormID Fetch(const FormID baseFormID, const std::string& baseEditorID,
                 const std::optional<uint32_t> customID) {
        auto* base_form = GetFormByID(baseFormID, baseEditorID);

        if (!base_form) {
            logger::error("Failed to get base form.");
            return 0;
        }

        if (customID.has_value()) {
            const auto new_formid = GetByCustomID(customID.value(), baseFormID, baseEditorID);
            if (const auto dyn_form = _yield(new_formid, base_form)) return dyn_form->GetFormID();
        } 
        if (const auto formset = GetFormSet(baseFormID, baseEditorID); !formset.empty()) {
            for (const auto _formid : formset) {
                if (IsActive(_formid)) continue;
                if (const auto dyn_form = _yield(_formid, base_form)) return dyn_form->GetFormID();
            }
        }

        return 0;
    }

    template <typename T>
    FormID FetchCreate(const FormID baseFormID, const std::string baseEditorID,
                       const std::optional<uint32_t> customID) {

        // TODO merge with Fetch
        auto* base_form = GetFormByID<T>(baseFormID, baseEditorID);
        
        if (!base_form) {
			logger::error("Failed to get base form.");
			return 0;
		}

        if (customID.has_value()) {
            const auto new_formid = GetByCustomID(customID.value(), baseFormID, baseEditorID);
            if (const auto dyn_form = _yield(new_formid, base_form)) return dyn_form->GetFormID();
        }
        else if (const auto formset = GetFormSet(baseFormID, baseEditorID); !formset.empty()) {
		    for (const auto _formid : formset) {
                if (IsActive(_formid)) continue;
                if (const auto dyn_form = _yield(_formid, base_form)) return dyn_form->GetFormID();
                //else if (!GetFormByID(_formid)) Delete({baseFormID, baseEditorID}, _formid);
		    }
        }

		// before creating new one, try to find one from the bank without custom id
		if (const auto dyn_form = GetFormByID<T>(Fetch(baseFormID,baseEditorID,{}))) {
			return dyn_form->GetFormID();
        }

        if (const auto dyn_form = _yield(Create<T>(base_form), base_form)) {
            const auto new_formid = dyn_form->GetFormID();
            if (customID.has_value()) customIDforms[new_formid] = customID.value();
            return new_formid;
        }

        return 0;
    }

    [[maybe_unused]] void ReviveAll() {
        for (const auto& [base, formset] : forms) {
            auto* base_form = GetFormByID(base.first, base.second);
            if (!base_form) {
                logger::error("Failed to get base form.");
                continue;
            }
            for (const auto _formid : formset) {
                if (const auto dyn_form = _yield(_formid, base_form)) {
                    logger::info("Revived form with ID: {:x}", dyn_form->GetFormID());
                }
            }
        }
    }

    void Reserve(const FormID baseID, const std::string& baseEditorID,const FormID dynamic_formid) {
        if (auto lock = std::shared_lock(protected_forms_mutex); protected_forms.contains(dynamic_formid)) return;
        const auto base_form = GetFormByID(
            baseID, baseEditorID);
        if (!base_form) {
			logger::warn("Base form with ID {:x} not found in forms.", baseID);
			return;
		}
        if (!GetFormByID(dynamic_formid)) {
            logger::warn("Form with ID {:x} not found in forms.", dynamic_formid);
            return;
        } 
        const auto form = GetFormByID(dynamic_formid);
        if (!underlying_check(base_form, form)) {
            logger::warn("Underlying check failed for form with ID {:x}.", dynamic_formid);
            return;
        }
        ReviveDynamicForm(form, base_form);
		std::unique_lock lock(protected_forms_mutex);
		std::unique_lock lock2(forms_mutex);
        forms[{baseID, baseEditorID}].insert(dynamic_formid);
        protected_forms.insert(dynamic_formid);
	}

	void Unreserve(const FormID dynamic_formid) {
		std::unique_lock lock(protected_forms_mutex);
        protected_forms.erase(dynamic_formid);
	}

    size_t GetNDeleted() {
		std::shared_lock lock(deleted_forms_mutex);
		return deleted_forms.size();
	}

    void SendData() {
        logger::info("--------Sending data (DFT) ---------");
        Clear();

        {
			std::unique_lock lock(act_effs_mutex);
            act_effs.clear();
        }

        const auto act_eff_list = RE::PlayerCharacter::GetSingleton()->AsMagicTarget()->GetActiveEffectList();

        int n_act_effs = 0;
        std::set<FormID> act_effs_temp;
        for (auto it = act_eff_list->begin(); it != act_eff_list->end(); ++it) {
            if (const auto* act_eff = *it){
                if (const auto act_eff_formid = act_eff->spell->GetFormID(); active_forms.contains(act_eff_formid)) {
                    if (act_effs_temp.contains(act_eff_formid)) logger::warn("Active effect already exists in act effs.");
                    else n_act_effs++;
					std::shared_lock lock(customIDforms_mutex);
					std::unique_lock lock2(act_effs_mutex);
                    const uint32_t customid_temp = customIDforms.contains(act_eff_formid) ? customIDforms.at(act_eff_formid) : 0;
                    act_effs.push_back({.baseFormid= GetOGFormOfDynamic(act_eff_formid)->GetFormID(),
                                        .dynamicFormid= act_eff_formid,
                                        .elapsed= act_eff->elapsedSeconds,
                                        .custom_id= {false, customid_temp}});
                    act_effs_temp.insert(act_eff_formid);
				}
            }
        }

        int n_fakes = 0;
        for (const auto& [base_pair, dyn_formset] : forms) {
            const DFSaveDataLHS lhs({base_pair.first, base_pair.second});
            DFSaveDataRHS rhs;
			for (const auto dyn_formid : dyn_formset) {
                if (!IsActive(dyn_formid) && !IsProtected(dyn_formid)) logger::info("Inactive form {:x} found in forms set.",dyn_formid);
				std::shared_lock lock(customIDforms_mutex);
				const auto has_customid = customIDforms.contains(dyn_formid);
                const uint32_t customid = has_customid ? customIDforms.at(dyn_formid) : 0;
                const float act_eff_elpsd = GetActiveEffectElapsed(dyn_formid);
                DFSaveData saveData({.dyn_formid= dyn_formid, .custom_id= {has_customid, customid}, .acteff_elapsed=
                                     act_eff_elpsd});
                rhs.push_back(saveData);
                n_fakes++;
			}
            if (!rhs.empty()) SetData(lhs, rhs);
        }

        logger::info("Number of dynamic forms sent: {}", n_fakes);
        logger::info("Number of active effects sent: {}", n_act_effs);
        logger::info("--------Data sent (DFT) ---------");
    }

    void ReceiveData() {
        // std::lock_guard<std::mutex> lock(mutex);
		logger::info("--------Receiving data (DFT) ---------");

        int n_fakes = 0;
        int n_act_effs = 0;
        for (const auto& [lhs, rhs] : m_Data) {
            auto base_formid = lhs.first;
            const auto& base_editorid = lhs.second;
            const auto temp_form = GetFormByID(0, base_editorid);
            if (!temp_form) logger::critical("Failed to get base form.");
            else base_formid = temp_form->GetFormID();
            for (const auto& [dyn_formid, custom_id, act_eff_elpsd] : rhs) {
                const auto [has_customid, customid] = custom_id;
                if (act_eff_elpsd >= 0.f) {
					std::unique_lock lock(act_effs_mutex);
                    act_effs.push_back({.baseFormid= base_formid, .dynamicFormid= dyn_formid, .elapsed= act_eff_elpsd, .custom_id=
                                        {has_customid, customid}});
                    n_act_effs++;
                }
                if (const auto dyn_form = RE::TESForm::LookupByID(dyn_formid); !dyn_form) {
                    logger::info("Dynamic form {:x} does not exist.", dyn_formid);
                    continue;
                }
                else if (const auto dyn_form_ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(dyn_formid)) {
                    logger::info("Dynamic form {:x} is a refr with name {}.", dyn_formid, dyn_form->GetName());
                    continue;
                }
                else if (!underlying_check(temp_form, dyn_form)) {
                    // bcs load callback happens after the game loads, there is a chance that the game will assign new
                    // stuff to "previously" our dynamic formid especially for stuff like dynamic food which is not
                    // serialized by the game
                    logger::trace("Underlying check failed for dynamic form {:x} with name {}.", dyn_formid,
                                  dyn_form->GetName());
                    continue;
                }

                if (auto lock = std::unique_lock(forms_mutex); forms.contains({base_formid, base_editorid}) &&
                    forms.at({base_formid, base_editorid}).contains(dyn_formid)) {
                    logger::trace("Form with ID {:x} already exist for baseid {} and editorid {}.", dyn_formid,
                                 base_formid, base_editorid);
                }
				else if (!forms[{base_formid, base_editorid}].insert(dyn_formid).second) {
					logger::error("Failed to insert new form into forms.");
					continue;
				}
				if (auto lock = std::unique_lock(customIDforms_mutex); has_customid) customIDforms[dyn_formid] = customid;
                n_fakes++;
			}
		}

        logger::info("Number of dynamic forms received: {}", n_fakes);
        logger::info("Number of active effects received: {}", n_act_effs);
        // need to check if formids and editorids are valid
#ifndef NDEBUG
        Print();
#endif  // !NDEBUG

        CleanseFormsets();

        logger::info("--------Data received (DFT) ---------");

	}

    void Reset() {
		// std::lock_guard<std::mutex> lock(mutex);
		//forms.clear();
        CleanseFormsets();
		std::unique_lock lock(customIDforms_mutex);
		std::unique_lock lock2(active_forms_mutex);
		std::unique_lock lock3(protected_forms_mutex);
		std::unique_lock lock4(act_effs_mutex);
		customIDforms.clear();
		active_forms.clear();
        protected_forms.clear();

        //deleted_forms.clear();

		act_effs.clear();
        block_create = false;
	}

    void Print() {
		std::shared_lock lock(forms_mutex);
        for (const auto& [base, formset] : forms) {
			logger::info("---------------------Base formid: {:x}, EditorID: {}---------------------", base.first, base.second);
			for (const auto _formid : formset) {
				logger::info("Dynamic formid: {:x} with name: {}", _formid, GetFormByID(_formid)->GetName());
			}
		}
    }

    void ApplyMissingActiveEffects() {

        std::map<FormID, float> new_act_effs; // terrible name
        // I need to change the formids in act_effs if they are not valid to valid ones
        for (std::shared_lock lock(act_effs_mutex);
            auto& [baseFormid, dynamicFormid, elapsed, customid] : act_effs) {
            if (elapsed < 0.f) {
				logger::error("Elapsed time is negative. Removing from act effs.");
				continue;
			}
            const auto& [has_cstmid, custom_id] = customid;
            const auto base_mg_item = GetFormByID(baseFormid);
            if (!base_mg_item) {
				logger::error("Failed to get base form.");
				continue;
			}
            if (!has_cstmid) {
                new_act_effs[dynamicFormid] = elapsed;
                continue;
            }
            const auto dyn_formid = GetByCustomID(custom_id, baseFormid, clib_util::editorID::get_editorID(base_mg_item));
            if (!dyn_formid) {
                logger::error("Failed to get form by custom id. Removing from act effs.");
                continue;
            }
			new_act_effs[dyn_formid] = elapsed;
		}
        {
            std::unique_lock lock(act_effs_mutex);
            act_effs.clear();
        }
        if (new_act_effs.empty()) return;

        const auto plyr = RE::PlayerCharacter::GetSingleton();
        const auto mg_target = plyr->AsMagicTarget();
        if (!mg_target) {
            logger::error("Failed to get player as magic target.");
            return;
        }
        auto act_eff_list = mg_target->GetActiveEffectList();
        for (auto it = act_eff_list->begin(); it != act_eff_list->end(); ++it) {
            if (const auto* act_eff = *it) {
                if (const auto mg_item = act_eff->spell) {
                    const auto mg_item_formid = mg_item->GetFormID();
                    if (new_act_effs.contains(mg_item_formid)) new_act_effs.erase(mg_item_formid);
                }
            }
        }

        const auto mg_caster = plyr->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);
        if (!mg_caster) {
            logger::error("Failed to get player as magic caster.");
            return;
        }
        for (const auto& item_formid : new_act_effs | std::views::keys) {
            auto* item = RE::TESForm::LookupByID<RE::MagicItem>(item_formid);
            if (!item) {
                logger::error("Failed to get item by formid.");
                continue;
            }
            mg_caster->CastSpellImmediate(item, false, plyr, 1.0f, false, 0.0f, nullptr);
        }

        // now i need to go to act eff list and adjust the elapsed time
        act_eff_list = mg_target->GetActiveEffectList();
        for (auto it = act_eff_list->begin(); it != act_eff_list->end(); ++it) {
            if (auto* act_eff = *it) {
                if (const auto mg_item = act_eff->spell) {
                    const auto mg_item_formid = mg_item->GetFormID();
                    if (new_act_effs.contains(mg_item_formid)) {
                        if (act_eff->duration > new_act_effs[mg_item_formid]) {
                            act_eff->elapsedSeconds = new_act_effs[mg_item_formid];
                        } else {
                            act_eff->elapsedSeconds = act_eff->duration - 1;
                        }
                        new_act_effs.erase(mg_item_formid);
                    }
                }
            }
        }

    }
};
