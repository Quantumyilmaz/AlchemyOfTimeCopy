#include "DrawDebug.h"

#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>

namespace DebugAPI_IMPL {
    glm::highp_mat4 GetRotationMatrix(const glm::vec3 eulerAngles) {
        return glm::eulerAngleXYZ(-(eulerAngles.x), -(eulerAngles.y), -(eulerAngles.z));
    }

    glm::vec3 NormalizeVector(const glm::vec3 p) { return glm::normalize(p); }

    glm::vec3 RotateVector(const glm::quat quatIn, const glm::vec3 vecIn) {
        const float num = quatIn.x * 2.0f;
        const float num2 = quatIn.y * 2.0f;
        const float num3 = quatIn.z * 2.0f;
        const float num4 = quatIn.x * num;
        const float num5 = quatIn.y * num2;
        const float num6 = quatIn.z * num3;
        const float num7 = quatIn.x * num2;
        const float num8 = quatIn.x * num3;
        const float num9 = quatIn.y * num3;
        const float num10 = quatIn.w * num;
        const float num11 = quatIn.w * num2;
        const float num12 = quatIn.w * num3;
        glm::vec3 result;
        result.x = (1.0f - (num5 + num6)) * vecIn.x + (num7 - num12) * vecIn.y + (num8 + num11) * vecIn.z;
        result.y = (num7 + num12) * vecIn.x + (1.0f - (num4 + num6)) * vecIn.y + (num9 - num10) * vecIn.z;
        result.z = (num8 - num11) * vecIn.x + (num9 + num10) * vecIn.y + (1.0f - (num4 + num5)) * vecIn.z;
        return result;
    }

    glm::vec3 RotateVector(const glm::vec3 eulerIn, const glm::vec3 vecIn) {
        const glm::vec3 glmVecIn(vecIn.x, vecIn.y, vecIn.z);
        const glm::mat3 rotationMatrix = glm::eulerAngleXYZ(eulerIn.x, eulerIn.y, eulerIn.z);

        return rotationMatrix * glmVecIn;
    }

    glm::vec3 GetForwardVector(const glm::quat quatIn) {
        // rotate Skyrim's base forward vector (positive Y forward) by quaternion
        return RotateVector(quatIn, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    glm::vec3 GetForwardVector(const glm::vec3 eulerIn) {
        const float pitch = eulerIn.x;
        const float yaw = eulerIn.z;

        return glm::vec3(sin(yaw) * cos(pitch), cos(yaw) * cos(pitch), sin(pitch));
    }

    glm::vec3 GetRightVector(const glm::quat quatIn) {
        // rotate Skyrim's base right vector (positive X forward) by quaternion
        return RotateVector(quatIn, glm::vec3(1.0f, 0.0f, 0.0f));
    }

    glm::vec3 GetRightVector(const glm::vec3 eulerIn) {
        const float pitch = eulerIn.x;
        const float yaw = eulerIn.z + glm::half_pi<float>();

        return glm::vec3(sin(yaw) * cos(pitch), cos(yaw) * cos(pitch), sin(pitch));
    }

    glm::vec3 ThreeAxisRotation(const float r11, const float r12, const float r21, const float r31, const float r32) {
        return glm::vec3(asin(r21), atan2(r11, r12), atan2(-r31, r32));
    }

    glm::vec3 RotMatrixToEuler(RE::NiMatrix3 matrixIn) {
        auto ent = matrixIn.entry;
        const auto rotMat = glm::mat4(
            {ent[0][0], ent[1][0], ent[2][0], ent[0][1], ent[1][1], ent[2][1], ent[0][2], ent[1][2], ent[2][2]});

        glm::vec3 rotOut;
        glm::extractEulerAngleXYZ(rotMat, rotOut.x, rotOut.y, rotOut.z);

        return rotOut;
    }

    constexpr int FIND_COLLISION_MAX_RECURSION = 2;

    RE::NiAVObject* GetCharacterSpine(const RE::TESObjectREFR* object) {
        const auto characterObject = object->GetObjectReference()->As<RE::TESNPC>();
        const auto mesh = object->GetCurrent3D();

        if (characterObject && mesh) {
            const auto spineNode = mesh->GetObjectByName("NPC Spine [Spn0]");
            if (spineNode) return spineNode;
        }

        return mesh;
    }

    RE::NiAVObject* GetCharacterHead(const RE::TESObjectREFR* object) {
        const auto characterObject = object->GetObjectReference()->As<RE::TESNPC>();
        const auto mesh = object->GetCurrent3D();

        if (characterObject && mesh) {
            const auto spineNode = mesh->GetObjectByName("NPC Head [Head]");
            if (spineNode) return spineNode;
        }

        return mesh;
    }

    bool IsRoughlyEqual(const float first, const float second, const float maxDif) { return abs(first - second) <= maxDif; }

    glm::vec3 QuatToEuler(const glm::quat q) {
        const auto matrix = glm::toMat4(q);

        glm::vec3 rotOut;
        glm::extractEulerAngleXYZ(matrix, rotOut.x, rotOut.y, rotOut.z);

        return rotOut;
    }

    glm::quat EulerToQuat(const glm::vec3 rotIn) {
        const auto matrix = glm::eulerAngleXYZ(rotIn.x, rotIn.y, rotIn.z);
        return glm::toQuat(matrix);
    }

    glm::vec3 GetInverseRotation(const glm::vec3 rotIn) {
        const auto matrix = glm::eulerAngleXYZ(rotIn.y, rotIn.x, rotIn.z);
        const auto inverseMatrix = glm::inverse(matrix);

        glm::vec3 rotOut;
        glm::extractEulerAngleYXZ(inverseMatrix, rotOut.x, rotOut.y, rotOut.z);
        return rotOut;
    }

    glm::quat GetInverseRotation(const glm::quat rotIn) { return glm::inverse(rotIn); }

    glm::vec3 EulerRotationToVector(const glm::vec3 rotIn) {
        return glm::vec3(cos(rotIn.y) * cos(rotIn.x), sin(rotIn.y) * cos(rotIn.x), sin(rotIn.x));
    }

    glm::vec3 VectorToEulerRotation(const glm::vec3 vecIn) {
        const float yaw = atan2(vecIn.x, vecIn.y);
        const float pitch = atan2(vecIn.z, sqrt((vecIn.x * vecIn.x) + (vecIn.y * vecIn.y)));

        return glm::vec3(pitch, 0.0f, yaw);
    }

    glm::vec3 GetCameraPos() {
        const auto playerCam = RE::PlayerCamera::GetSingleton();
        return glm::vec3(playerCam->pos.x, playerCam->pos.y, playerCam->pos.z);
    }

    glm::quat GetCameraRot() {
        const auto playerCam = RE::PlayerCamera::GetSingleton();

        const auto cameraState = playerCam->currentState.get();
        if (!cameraState) return glm::quat();

        RE::NiQuaternion niRotation;
        cameraState->GetRotation(niRotation);

        return glm::quat(niRotation.w, niRotation.x, niRotation.y, niRotation.z);
    }

    bool IsPosBehindPlayerCamera(const glm::vec3 pos) {
        const auto cameraPos = GetCameraPos();
        const auto cameraRot = GetCameraRot();

        const auto toTarget = NormalizeVector(pos - cameraPos);
        const auto cameraForward = NormalizeVector(GetForwardVector(cameraRot));

        const auto angleDif = abs(glm::length(toTarget - cameraForward));

        // root_two is the diagonal length of a 1x1 square. When comparing normalized forward
        // vectors, this accepts an angle of 90 degrees in all directions
        return angleDif > glm::root_two<float>();
    }

    glm::vec3 GetPointOnRotatedCircle(const glm::vec3 origin, const float radius, const float i, const float maxI, const glm::vec3 eulerAngles) {
        const float currAngle = (i / maxI) * glm::two_pi<float>();

        const glm::vec3 targetPos((radius * cos(currAngle)), (radius * sin(currAngle)), 0.0f);

        const auto targetPosRotated = RotateVector(eulerAngles, targetPos);

        return glm::vec3(targetPosRotated.x + origin.x, targetPosRotated.y + origin.y, targetPosRotated.z + origin.z);
    }

    glm::vec3 GetObjectAccuratePosition(const RE::TESObjectREFR* object) {
        const auto mesh = object->GetCurrent3D();

        // backup, if no mesh is found
        if (!mesh) {
            const auto niPos = object->GetPosition();
            return glm::vec3(niPos.x, niPos.y, niPos.z);
        }

        const auto niPos = mesh->world.translate;
        return glm::vec3(niPos.x, niPos.y, niPos.z);
    }

    bool DebugAPI::CachedMenuData;

    float DebugAPI::ScreenResX;
    float DebugAPI::ScreenResY;

    DebugAPILine::DebugAPILine(glm::vec3 from, glm::vec3 to, glm::vec4 color, float lineThickness,
                               unsigned __int64 destroyTickCount) {
        From = from;
        To = to;
        Color = color;
        fColor = DebugAPI::RGBToHex(color);
        Alpha = color.a * 100.0f;
        LineThickness = lineThickness;
        DestroyTickCount = destroyTickCount;
    }

    void DebugAPI::DrawLineForMS(const glm::vec3& from, const glm::vec3& to, const int liftetimeMS, const glm::vec4& color,
                                 const float lineThickness) {
        if (DebugAPILine* oldLine = GetExistingLine(from, to, color, lineThickness)) {
            oldLine->From = from;
            oldLine->To = to;
            oldLine->DestroyTickCount = GetTickCount64() + liftetimeMS;
            oldLine->LineThickness = lineThickness;
            return;
        }

        const auto newLine = new DebugAPILine(from, to, color, lineThickness, GetTickCount64() + liftetimeMS);
		std::unique_lock lock(mutex_);
        LinesToDraw.push_back(newLine);
    }

    void DebugAPI::Update() {
        const auto hud = GetHUD();
        if (!hud || !hud->uiMovie) return;

        CacheMenuData();
        ClearLines2D(hud->uiMovie);

		std::unique_lock lock(mutex_);
        for (int i = 0; i < LinesToDraw.size(); i++) {
            const DebugAPILine* line = LinesToDraw[i];

            DrawLine3D(hud->uiMovie, line->From, line->To, line->fColor, line->LineThickness, line->Alpha);

            if (GetTickCount64() > line->DestroyTickCount) {
                LinesToDraw.erase(LinesToDraw.begin() + i);
                delete line;
                i--;
            }
        }
    }

    void DebugAPI::DrawSphere(const glm::vec3 origin, const float radius, const int liftetimeMS, const glm::vec4& color,
                              const float lineThickness) {
        DrawCircle(origin, radius, glm::vec3(0.0f, 0.0f, 0.0f), liftetimeMS, color, lineThickness);
        DrawCircle(origin, radius, glm::vec3(glm::half_pi<float>(), 0.0f, 0.0f), liftetimeMS, color, lineThickness);
    }

    void DebugAPI::DrawCircle(const glm::vec3 origin, const float radius, const glm::vec3 eulerAngles, const int liftetimeMS,
                              const glm::vec4& color, const float lineThickness) {
        glm::vec3 lastEndPos =
            GetPointOnRotatedCircle(origin, radius, CIRCLE_NUM_SEGMENTS, (float)(CIRCLE_NUM_SEGMENTS - 1), eulerAngles);

        for (int i = 0; i <= CIRCLE_NUM_SEGMENTS; i++) {
            glm::vec3 currEndPos =
                GetPointOnRotatedCircle(origin, radius, (float)i, (float)(CIRCLE_NUM_SEGMENTS - 1), eulerAngles);

            DrawLineForMS(lastEndPos, currEndPos, liftetimeMS, color, lineThickness);

            lastEndPos = currEndPos;
        }
    }

    DebugAPILine* DebugAPI::GetExistingLine(const glm::vec3& from, const glm::vec3& to, const glm::vec4& color,
                                            const float lineThickness) {
		std::shared_lock lock(mutex_);
        for (int i = 0; i < LinesToDraw.size(); i++) {
            DebugAPILine* line = LinesToDraw[i];

            if (IsRoughlyEqual(from.x, line->From.x, DRAW_LOC_MAX_DIF) &&
                IsRoughlyEqual(from.y, line->From.y, DRAW_LOC_MAX_DIF) &&
                IsRoughlyEqual(from.z, line->From.z, DRAW_LOC_MAX_DIF) &&
                IsRoughlyEqual(to.x, line->To.x, DRAW_LOC_MAX_DIF) &&
                IsRoughlyEqual(to.y, line->To.y, DRAW_LOC_MAX_DIF) &&
                IsRoughlyEqual(to.z, line->To.z, DRAW_LOC_MAX_DIF) &&
                IsRoughlyEqual(lineThickness, line->LineThickness, DRAW_LOC_MAX_DIF) && color == line->Color) {
                return line;
            }
        }

        return nullptr;
    }

    void DebugAPI::DrawLine3D(const RE::GPtr<RE::GFxMovieView>& movie, const glm::vec3 from, const glm::vec3 to, const float color,
                              const float lineThickness, const float alpha) {
        if (IsPosBehindPlayerCamera(from) && IsPosBehindPlayerCamera(to)) return;

        const glm::vec2 screenLocFrom = WorldToScreenLoc(movie, from);
        const glm::vec2 screenLocTo = WorldToScreenLoc(movie, to);
        DrawLine2D(movie, screenLocFrom, screenLocTo, color, lineThickness, alpha);
    }

    void DebugAPI::DrawLine3D(const RE::GPtr<RE::GFxMovieView>& movie, const glm::vec3 from, const glm::vec3 to, const glm::vec4 color,
                              const float lineThickness) {
        DrawLine3D(movie, from, to, RGBToHex(glm::vec3(color.r, color.g, color.b)), lineThickness, color.a * 100.0f);
    }

    void DebugAPI::DrawLine2D(const RE::GPtr<RE::GFxMovieView>& movie, glm::vec2 from, glm::vec2 to, const float color,
                              const float lineThickness, const float alpha) {
        // all parts of the line are off screen - don't need to draw them
        if (!IsOnScreen(from, to)) return;

        FastClampToScreen(from);
        FastClampToScreen(to);

        // lineStyle(thickness:Number = NaN, color : uint = 0, alpha : Number = 1.0, pixelHinting : Boolean = false,
        // scaleMode : String = "normal", caps : String = null, joints : String = null, miterLimit : Number = 3) :void
        //
        // CapsStyle values: 'NONE', 'ROUND', 'SQUARE'
        // const char* capsStyle = "NONE";

        const RE::GFxValue argsLineStyle[3]{lineThickness, color, alpha};
        movie->Invoke("lineStyle", nullptr, argsLineStyle, 3);

        const RE::GFxValue argsStartPos[2]{from.x, from.y};
        movie->Invoke("moveTo", nullptr, argsStartPos, 2);

        const RE::GFxValue argsEndPos[2]{to.x, to.y};
        movie->Invoke("lineTo", nullptr, argsEndPos, 2);

        movie->Invoke("endFill", nullptr, nullptr, 0);
    }

    void DebugAPI::DrawLine2D(const RE::GPtr<RE::GFxMovieView>& movie, const glm::vec2 from, const glm::vec2 to, const glm::vec4 color,
                              const float lineThickness) {
        DrawLine2D(movie, from, to, RGBToHex(glm::vec3(color.r, color.g, color.b)), lineThickness, color.a * 100.0f);
    }

    void DebugAPI::ClearLines2D(const RE::GPtr<RE::GFxMovieView>& movie) { movie->Invoke("clear", nullptr, nullptr, 0); }

    RE::GPtr<RE::IMenu> DebugAPI::GetHUD() {
        RE::GPtr<RE::IMenu> hud = RE::UI::GetSingleton()->GetMenu(DebugOverlayMenu::MENU_NAME);
        return hud;
    }

    float DebugAPI::RGBToHex(const glm::vec3 rgb) {
        return ConvertComponentR(rgb.r * 255) + ConvertComponentG(rgb.g * 255) + ConvertComponentB(rgb.b * 255);
    }

    // if drawing outside the screen rect, at some point Scaleform seems to start resizing the rect internally, without
    // increasing resolution. This will cause all line draws to become more and more pixelated and increase in thickness
    // the farther off screen even one line draw goes. I'm allowing some leeway, then I just clamp the
    // coordinates to the screen rect.
    //
    // this is inaccurate. A more accurate solution would require finding the sub vector that overshoots the screen rect
    // between two points and scale the vector accordingly. Might implement that at some point, but the inaccuracy is
    // barely noticeable
    const float CLAMP_MAX_OVERSHOOT = 10000.0f;
    void DebugAPI::FastClampToScreen(glm::vec2& point) {
        if (point.x < 0.0) {
            const float overshootX = abs(point.x);
            if (overshootX > CLAMP_MAX_OVERSHOOT) point.x += overshootX - CLAMP_MAX_OVERSHOOT;
        } else if (point.x > ScreenResX) {
            const float overshootX = point.x - ScreenResX;
            if (overshootX > CLAMP_MAX_OVERSHOOT) point.x -= overshootX - CLAMP_MAX_OVERSHOOT;
        }

        if (point.y < 0.0) {
            const float overshootY = abs(point.y);
            if (overshootY > CLAMP_MAX_OVERSHOOT) point.y += overshootY - CLAMP_MAX_OVERSHOOT;
        } else if (point.y > ScreenResY) {
            const float overshootY = point.y - ScreenResY;
            if (overshootY > CLAMP_MAX_OVERSHOOT) point.y -= overshootY - CLAMP_MAX_OVERSHOOT;
        }
    }

    float DebugAPI::ConvertComponentR(const float value) { return (value * 0xffff) + value; }

    float DebugAPI::ConvertComponentG(const float value) { return (value * 0xff) + value; }

    float DebugAPI::ConvertComponentB(const float value) { return value; }

    glm::vec2 DebugAPI::WorldToScreenLoc(const RE::GPtr<RE::GFxMovieView>& movie, const glm::vec3 worldLoc) {
        static uintptr_t g_worldToCamMatrix = RELOCATION_ID(519579, 406126).address();  // 2F4C910, 2FE75F0
        static auto g_viewPort =
            (RE::NiRect<float>*)RELOCATION_ID(519618, 406160).address();  // 2F4DED0, 2FE8B98

        glm::vec2 screenLocOut;
        const RE::NiPoint3 niWorldLoc(worldLoc.x, worldLoc.y, worldLoc.z);

        float zVal;

        RE::NiCamera::WorldPtToScreenPt3((float(*)[4])g_worldToCamMatrix, *g_viewPort, niWorldLoc, screenLocOut.x,
                                         screenLocOut.y, zVal, 1e-5f);
        const RE::GRectF rect = movie->GetVisibleFrameRect();

        screenLocOut.x = rect.left + (rect.right - rect.left) * screenLocOut.x;
        screenLocOut.y = 1.0f - screenLocOut.y;  // Flip y for Flash coordinate system
        screenLocOut.y = rect.top + (rect.bottom - rect.top) * screenLocOut.y;

        return screenLocOut;
    }

    DebugOverlayMenu::DebugOverlayMenu() {
        const auto scaleformManager = RE::BSScaleformManager::GetSingleton();

        inputContext = Context::kNone;
        depthPriority = 127;

        menuFlags.set(RE::UI_MENU_FLAGS::kRequiresUpdate);
        menuFlags.set(RE::UI_MENU_FLAGS::kAllowSaving);
        menuFlags.set(RE::UI_MENU_FLAGS::kCustomRendering);

        scaleformManager->LoadMovieEx(this, MENU_PATH, [](RE::GFxMovieDef* a_def) -> void {
            a_def->SetState(RE::GFxState::StateType::kLog, RE::make_gptr<Logger>().get());
        });
    }

    void DebugOverlayMenu::Register() {
        const auto ui = RE::UI::GetSingleton();
        if (ui) {
            ui->Register(MENU_NAME, Creator);

            DebugOverlayMenu::Show();
        }
    }

    void DebugOverlayMenu::Show() {
        const auto msgQ = RE::UIMessageQueue::GetSingleton();
        if (msgQ) {
            msgQ->AddMessage(DebugOverlayMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kShow, nullptr);
        }
    }

    void DebugOverlayMenu::Hide() {
        const auto msgQ = RE::UIMessageQueue::GetSingleton();
        if (msgQ) {
            msgQ->AddMessage(DebugOverlayMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kHide, nullptr);
        }
    }

    void DebugAPI::CacheMenuData() {
        if (CachedMenuData) return;

        const RE::GPtr<RE::IMenu> menu = RE::UI::GetSingleton()->GetMenu(DebugOverlayMenu::MENU_NAME);
        if (!menu || !menu->uiMovie) return;

        const RE::GRectF rect = menu->uiMovie->GetVisibleFrameRect();

        ScreenResX = abs(rect.left - rect.right);
        ScreenResY = abs(rect.top - rect.bottom);

        CachedMenuData = true;
    }

    bool DebugAPI::IsOnScreen(const glm::vec2 from, const glm::vec2 to) { return IsOnScreen(from) || IsOnScreen(to); }

    bool DebugAPI::IsOnScreen(const glm::vec2 point) {
        return (point.x <= ScreenResX && point.x >= 0.0 && point.y <= ScreenResY && point.y >= 0.0);
    }

    void DebugOverlayMenu::AdvanceMovie(const float a_interval, const std::uint32_t a_currentTime) {
        RE::IMenu::AdvanceMovie(a_interval, a_currentTime);

        DebugAPI::Update();
    }
}
