#include "Gui.h"
#include "imgui/imgui.h"
#include <vector>
#include "model/ModelInstance.h"
#include "renderer/simple_renderer/SimpleRenderer.h"
#include <string>
#include <iostream>
#include <glm/glm.hpp>
#include <map>
#include "post/PostProcessing.h"
#include "renderer/shadow/Shadow.h"
#include "window/Window.h"
#include "post/GaussianBlur.h"
#include "post/bloom/Bloom.h"
#include "renderer/Skybox.h"

static void GeneralGui();
static void ModelGui();
static void SceneGui();
static void ModelInstanceGui();
static void LightGui();
static void ShadowGui();
static void PostProcessingGui();

void Gui::buildSimpleRendererGui() {
	// ImGui::ShowDemoWindow();
	// FPS counter.
	ImGui::Text((std::string("FPS: ") + std::to_string(Window::getFps()).c_str()).c_str());
	GeneralGui();
	ModelGui();
	SceneGui();
	ModelInstanceGui();
	LightGui();
	ShadowGui();
	PostProcessingGui();
}

static void GeneralGui() {
	if (ImGui::CollapsingHeader("General##general")) {
		bool usePbr = SimpleRenderer::getUsePbr();
		if (ImGui::Checkbox("Use pbr##usepbr", &usePbr))
			SimpleRenderer::setUsePbr(usePbr);
		bool useSkybox = Skybox::getUseSkybox();
		if (ImGui::Checkbox("Use skybox##sky", &useSkybox))
			Skybox::setUseSkybox(useSkybox);
	}
}

static void ModelGui() {
	if (ImGui::CollapsingHeader("Model##model")) {
		// Read text and load model.
		// If name is wrong or doesn't exist everything goes to shit.
		static char char_buff[64] = "";
		ImGui::PushItemWidth(ImGui::GetFontSize() * 12);
		ImGui::InputText("##modelinputtext", char_buff, ((int)sizeof(char_buff))/((int)sizeof(*(char_buff)))); // Size of static array? Look in imgui_demo.cpp.
		ImGui::PopItemWidth();
		ImGui::SameLine();
		if (ImGui::Button("Load model##model")) {
			// Should add a function that only does the second part of getAssetModel().
			getAssetModel(char_buff);
		}
		
		std::vector<std::string> items;
		std::map<std::string, Model>& models = getModels();
		for (auto& pair : models) {
			items.push_back(pair.first);
		}

		// If there are no models don't go forward.
		if (items.size() == 0)
			return;

		static int item_current_idx = 0; // Here we store our selection data as an index.
		static int mesh_item_current_idx = 0; // Here we store our selection data as an index.
		const char* combo_preview_value = items[item_current_idx].c_str();  // Pass in the preview value visible before opening the combo (it could be anything)

		if (ImGui::BeginCombo("Model##combo", combo_preview_value))
		{
			for (int n = 0; n < items.size(); n++)
			{
				const bool is_selected = (item_current_idx == n);
				if (ImGui::Selectable(items[n].c_str(), is_selected)) {
					item_current_idx = n;
					mesh_item_current_idx = 0;
				}

				// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		Model& m = models.at(items[item_current_idx]);

		std::vector<std::string> meshes;
		for (int i = 0; i < m.getMeshes().size(); ++i) {
			meshes.push_back("Mesh " + std::to_string(i));
		}

		// If there are no meshes don't go forward.
		if (meshes.size() == 0)
			return;

		// Mesh combo box.
		const char* mesh_combo_preview_value = meshes[mesh_item_current_idx].c_str();  // Pass in the preview value visible before opening the combo (it could be anything)

		if (ImGui::BeginCombo("Mesh##meshcombo", mesh_combo_preview_value))
		{
			for (int n = 0; n < meshes.size(); n++)
			{
				const bool mesh_is_selected = (mesh_item_current_idx == n);
				if (ImGui::Selectable(meshes[n].c_str(), mesh_is_selected))
					mesh_item_current_idx = n;

				// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
				if (mesh_is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		// End of mesh combo box.

		Mesh& m_mesh = m.getMeshes()[mesh_item_current_idx];

		float amb[3] = { m_mesh.material.ambient.x, m_mesh.material.ambient.y, m_mesh.material.ambient.z };
		float dif[3] = { m_mesh.material.diffuse.x, m_mesh.material.diffuse.y, m_mesh.material.diffuse.z };
		float spe[3] = { m_mesh.material.specular.x, m_mesh.material.specular.y, m_mesh.material.specular.z };
		float shi = m_mesh.material.shininess, rou = m_mesh.material.roughness, met = m_mesh.material.metallic;
		ImGui::DragFloat3("Ambient##model", &amb[0], 0.005f, 0.0f, 100.0f);
		ImGui::DragFloat3("Diffuse##model", &dif[0], 0.005f, 0.0f, 100.0f);
		ImGui::DragFloat3("Specular##model", &spe[0], 0.005f, 0.0f, 100.0f);
		ImGui::DragFloat("Shininess##model", &shi, 0.005f);
		ImGui::DragFloat("Roughness##model", &rou, 0.001f, 0.0f, 1.0f);
		ImGui::DragFloat("Metallic##model", &met, 0.001f, 0.0f, 1.0f);
		m_mesh.material.ambient = glm::vec3(amb[0], amb[1], amb[2]);
		m_mesh.material.diffuse = glm::vec3(dif[0], dif[1], dif[2]);
		m_mesh.material.specular = glm::vec3(spe[0], spe[1], spe[2]);
		m_mesh.material.shininess = shi;
		m_mesh.material.roughness = rou;
		m_mesh.material.metallic = met;
	}
}

static void SceneGui() {
	if (ImGui::CollapsingHeader("Scene##scene")) {
		if (ImGui::Button("Save current scene##scene")) {
			Scene& scene = SimpleRenderer::getScene();
			scene.save();
		}
		// Read text and switch scene.
		// If name is wrong or doesn't exist new scene will be created.
		static char char_buff[64] = "";
		ImGui::PushItemWidth(ImGui::GetFontSize() * 12);
		ImGui::InputText("##sceneinputtext", char_buff, ((int)sizeof(char_buff)) / ((int)sizeof(*(char_buff)))); // Size of static array? Look in imgui_demo.cpp.
		ImGui::PopItemWidth();
		ImGui::SameLine();
		if (ImGui::Button("Switch scene##scene")) {
			Scene s = Scene(char_buff);
			// Terminate old scene.
			SimpleRenderer::getScene().terminate();
			// Set and initialize new scene.
			SimpleRenderer::setScene(s);
			SimpleRenderer::getScene().initialize();
		}
	}
}

static void ModelInstanceGui() {
	if (ImGui::CollapsingHeader("ModelInstance##modelinstance")) {

		// If there are no models don't go forward.
		if (getModels().size() == 0)
			return;

		std::vector<std::string> items;
		std::vector<ModelInstance>& modelInstances = SimpleRenderer::getScene().getModelInstancesManager().getModelInstances();
		for (int i = 0; i < modelInstances.size(); ++i) {
			items.push_back("ModelInstance " + std::to_string(i));
		}

		if (ImGui::Button("Add new model instance##mi")) {
			const auto& it = getModels().begin();
			SimpleRenderer::getScene().getModelInstancesManager().getModelInstances().push_back(ModelInstance(&it->second));
		}

		// If there are no model instances don't go forward.
		if (items.size() == 0)
			return;

		static int item_current_idx = 0; // Here we store our selection data as an index.
		static int model_item_current_idx = 0; // Here we store our selection data as an index.
		const char* combo_preview_value = items[item_current_idx].c_str();  // Pass in the preview value visible before opening the combo (it could be anything)

		static bool updateModelSelection = false;

		if (ImGui::BeginCombo("ModelInstance##combo", combo_preview_value))
		{
			for (int n = 0; n < items.size(); n++)
			{
				const bool is_selected = (item_current_idx == n);
				if (ImGui::Selectable(items[n].c_str(), is_selected)) {
					item_current_idx = n;
					updateModelSelection = true;
				}

				// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		ModelInstance& mi = modelInstances[item_current_idx];

		// Model selection.
		std::vector<std::string> model_items;
		std::map<std::string, Model>& models = getModels();
		int currentModelIndex = 0;
		for (auto& pair : models) {
			model_items.push_back(pair.first);
		}
		// If modelinstance in combo has been changed set the index of our current ModelInstance model.
		if (updateModelSelection) {
			for (int i = 0; i < model_items.size(); ++i) {
				if (model_items[i].compare(mi.getModel()->getName()) == 0)
					currentModelIndex = i;
			}
			model_item_current_idx = currentModelIndex;
			updateModelSelection = false;
		}
		const char* model_combo_preview_value = model_items[model_item_current_idx].c_str();  // Pass in the preview value visible before opening the combo (it could be anything)

		if (ImGui::BeginCombo("Model##micombo", model_combo_preview_value))
		{
			for (int n = 0; n < model_items.size(); n++)
			{
				const bool is_selected = (model_item_current_idx == n);
				if (ImGui::Selectable(model_items[n].c_str(), is_selected)) {
					model_item_current_idx = n;
					mi.setModel(getAssetModel(model_items[n]));
				}

				// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		Model& m = models.at(model_items[model_item_current_idx]);

		static float sliderLimit = 30;
		float pos[3] = { mi.getX(), mi.getY(), mi.getZ() };
		float rot[3] = { mi.getRotation().x, mi.getRotation().y, mi.getRotation().z };
		float sca[3] = { mi.getScale().x, mi.getScale().y, mi.getScale().z };
		ImGui::DragFloat3("Position##modelinstance", &pos[0], 0.005f);
		ImGui::DragFloat3("Rotation##modelinstance", &rot[0], 0.05f);
		ImGui::DragFloat3("Scale##modelinstance", &sca[0], 0.005f);
		mi.setPosition(glm::vec3(pos[0], pos[1], pos[2]));
		mi.setRotation(glm::vec3(rot[0], rot[1], rot[2]));
		mi.setScale(glm::vec3(sca[0], sca[1], sca[2]));
		if (ImGui::Button("Remove current model instance##mi")) {
			std::vector<ModelInstance>& modelInstances = SimpleRenderer::getScene().getModelInstancesManager().getModelInstances();
			modelInstances.erase(modelInstances.begin() + item_current_idx);
			item_current_idx > 0 ? item_current_idx-- : 0;
			updateModelSelection = true;
		}
	}
}

static void LightGui() {
	if (ImGui::CollapsingHeader("Light##light")) {

		if (ImGui::Button("Add new light##light")) {
			SimpleRenderer::getScene().getLightsManager().addLight(Light());
		}
		std::vector<std::string> items;
		std::vector<Light>& lights = SimpleRenderer::getScene().getLightsManager().getLights();
		SunLight& sunLight = SimpleRenderer::getScene().getLightsManager().getSunLight();
		items.push_back("SunLight");
		for (int i = 0; i < lights.size(); ++i) {
			items.push_back("Light " + std::to_string(i));
		}
		static int item_current_idx = 0; // Here we store our selection data as an index.
		const char* combo_preview_value = items[item_current_idx].c_str();  // Pass in the preview value visible before opening the combo (it could be anything)

		if (ImGui::BeginCombo("Light##combo", combo_preview_value))
		{
			for (int n = 0; n < items.size(); n++)
			{
				const bool is_selected = (item_current_idx == n);
				if (ImGui::Selectable(items[n].c_str(), is_selected))
					item_current_idx = n;

				// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		if (item_current_idx == 0) {
			SunLight& l = sunLight;
			static float sliderLimit = 30;
			static float lightSliderLimit = 1;
			float pos[3] = { l.getPosition().x, l.getPosition().y, l.getPosition().z };
			float amb[3] = { l.getAmbient().x, l.getAmbient().y, l.getAmbient().z };
			float dif[3] = { l.getDiffuse().x, l.getDiffuse().y, l.getDiffuse().z };
			float spe[3] = { l.getSpecular().x, l.getSpecular().y, l.getSpecular().z };
			bool useSunLight = SimpleRenderer::getScene().getLightsManager().getUseSunLight();
			if (ImGui::Checkbox("Use SunLight", &useSunLight))
				SimpleRenderer::getScene().getLightsManager().setUseSunLight(useSunLight);
			ImGui::DragFloat3("Position##light", &pos[0], 0.005f);
			ImGui::DragFloat3("Ambient##light", &amb[0], 0.005f, 0.0f, 100.0f);
			ImGui::DragFloat3("Diffuse##light", &dif[0], 0.005f, 0.0f, 100.0f);
			ImGui::DragFloat3("Specular##light", &spe[0], 0.005f, 0.0f, 100.0f);
			l.setPosition(glm::vec3(pos[0], pos[1], pos[2]));
			l.setAmbient(glm::vec3(amb[0], amb[1], amb[2]));
			l.setDiffuse(glm::vec3(dif[0], dif[1], dif[2]));
			l.setSpecular(glm::vec3(spe[0], spe[1], spe[2]));
		}
		else {
			Light& l = lights[item_current_idx - 1];
			static float sliderLimit = 30;
			static float lightSliderLimit = 10;
			float pos[3] = { l.getPosition().x, l.getPosition().y, l.getPosition().z };
			float amb[3] = { l.getAmbient().x, l.getAmbient().y, l.getAmbient().z };
			float dif[3] = { l.getDiffuse().x, l.getDiffuse().y, l.getDiffuse().z };
			float spe[3] = { l.getSpecular().x, l.getSpecular().y, l.getSpecular().z };
			float con = l.getConstant(), lin = l.getLinear(), qua = l.getQuadratic();
			ImGui::DragFloat3("Position##light", &pos[0], 0.005f);
			ImGui::DragFloat3("Ambient##light", &amb[0], 0.005f, 0.0f, 100.0f);
			ImGui::DragFloat3("Diffuse##light", &dif[0], 0.005f, 0.0f, 100.0f);
			ImGui::DragFloat3("Specular##light", &spe[0], 0.005f, 0.0f, 100.0f);
			ImGui::DragFloat("Constant##light", &con, 0.0005f);
			ImGui::DragFloat("Linear##light", &lin, 0.0005f);
			ImGui::DragFloat("Quadratic##light", &qua, 0.0005f);
			l.setPosition(glm::vec3(pos[0], pos[1], pos[2]));
			l.setAmbient(glm::vec3(amb[0], amb[1], amb[2]));
			l.setDiffuse(glm::vec3(dif[0], dif[1], dif[2]));
			l.setSpecular(glm::vec3(spe[0], spe[1], spe[2]));
			l.setConstant(con);
			l.setLinear(lin);
			l.setQuadratic(qua);
			if (ImGui::Button("Remove current light##light")) {
				SimpleRenderer::getScene().getLightsManager().removeLight(item_current_idx-1);
				item_current_idx > 0 ? item_current_idx--: 0;
			}
		}
	}
}

static void ShadowGui() {
	if (ImGui::CollapsingHeader("Shadow##shadow")) {
		bool useShadows = Shadow::getUseShadows();
		if (ImGui::Checkbox("Use shadows##shadow", &useShadows))
			Shadow::setUseShadows(useShadows);
		if (useShadows) {
			float shadowBiasMultiplier = Shadow::getShadowBiasMultiplier();
			ImGui::DragFloat("Shadow bias multiplier##shadow", &shadowBiasMultiplier, 0.00001f, 0.0f, 1.0f, " % .4f");
			Shadow::setShadowBiasMultiplier(shadowBiasMultiplier);

			float shadowBiasMinimum = Shadow::getShadowBiasMinimum();
			ImGui::DragFloat("Shadow bias minimum##shadow", &shadowBiasMinimum, 0.00001f, 0.0f, 1.0f, " % .4f");
			Shadow::setShadowBiasMinimum(shadowBiasMinimum);

			float csmPlanesDistanceInterpolationFactor = Shadow::getCsmPlanesDistanceInterpolationFactor();
			ImGui::DragFloat("Csm interpolation factor##shadow", &csmPlanesDistanceInterpolationFactor, 0.005f, 0.0f, 1.0f);
			Shadow::setCsmPlanesDistanceInterpolationFactor(csmPlanesDistanceInterpolationFactor);

			float csmBlendingOffset = Shadow::getCsmBlendingOffset();
			ImGui::DragFloat("Csm blending offset##shadow", &csmBlendingOffset, 0.005f);
			Shadow::setCsmBlendingOffset(csmBlendingOffset);

			bool usePcf = Shadow::getUsePcf(), usePoissonPcf = Shadow::getUsePoissonPcf();
			if (ImGui::Checkbox("Use pcf##shadow", &usePcf))
				Shadow::setUsePcf(usePcf);
			if (ImGui::Checkbox("Use poisson pcf##shadow", &usePoissonPcf))
				Shadow::setUsePoissonPcf(usePoissonPcf);
			if (usePoissonPcf) {
				int oldPoissonPcfSamplesNumber = Shadow::getPoissonPcfSamplesNumber(), poissonPcfSamplesNumber = oldPoissonPcfSamplesNumber;
				ImGui::DragInt("Poisson pcf samples number##shadow", &poissonPcfSamplesNumber, 0.05f, 2, 16);
				if (oldPoissonPcfSamplesNumber != poissonPcfSamplesNumber)
					Shadow::setPoissonPcfSamplesNumber(poissonPcfSamplesNumber);
				float poissonPcfDiameter = Shadow::getPoissonPcfDiameter();
				ImGui::DragFloat("Poisson pcf diameter", &poissonPcfDiameter, 0.005f);
				Shadow::setPoissonPcfDiameter(poissonPcfDiameter);
			}
		}
	}
}

static void PostProcessingGui() {
	if (ImGui::CollapsingHeader("Post processing##pp")) {
		bool usePostProcessing = PostProcessing::getUsePostProcessing();
		if (ImGui::Checkbox("Use post processing##pp", &usePostProcessing))
			PostProcessing::setUsePostProcessing(usePostProcessing);
		if (usePostProcessing) {

			std::vector<std::string> items;
			items.push_back("General"); // 0
			items.push_back("Bloom"); // 1
			items.push_back("Gaussian Blur"); // 2

			static int item_current_idx = 0; // Here we store our selection data as an index.
			const char* combo_preview_value = items[item_current_idx].c_str();  // Pass in the preview value visible before opening the combo (it could be anything)

			if (ImGui::BeginCombo("Section##ppcombo", combo_preview_value))
			{
				for (int n = 0; n < items.size(); n++)
				{
					const bool is_selected = (item_current_idx == n);
					if (ImGui::Selectable(items[n].c_str(), is_selected)) {
						item_current_idx = n;
					}

					// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			switch (item_current_idx) {
			case 0:
			{
				bool useBlackAndWhite = PostProcessing::getUseBlackAndWhite();
				if (ImGui::Checkbox("Use black and white##pp", &useBlackAndWhite))
					PostProcessing::setUseBlackAndWhite(useBlackAndWhite);
				bool useTonemapping = PostProcessing::getUseTonemapping(), useGammacorrection = PostProcessing::getUseGammacorrection();
				if (ImGui::Checkbox("Use tonemapping##pp", &useTonemapping))
					PostProcessing::setUseTonemapping(useTonemapping);
				if (useTonemapping) {
					float exposure = PostProcessing::getExposure();
					ImGui::DragFloat("Exposure##pp", &exposure, 0.005f);
					PostProcessing::setExposure(exposure);
				}
				if (ImGui::Checkbox("Use gamma correction##pp", &useGammacorrection))
					PostProcessing::setUseGammacorrection(useGammacorrection);
				if (useGammacorrection) {
					float gamma = PostProcessing::getGamma();
					ImGui::DragFloat("Gamma##pp", &gamma, 0.005f);
					PostProcessing::setGamma(gamma);
				}
			}
			break;
			case 1:
			{
				bool useBloom = PostProcessing::getUseBloom();
				if (ImGui::Checkbox("Use bloom##pp", &useBloom))
					PostProcessing::setUseBloom(useBloom);
				if (useBloom) {
					bool useBloomPrefilter = Bloom::getUsePrefilter();
					if (ImGui::Checkbox("Use bloom prefilter##pp", &useBloomPrefilter))
						Bloom::setUsePrefilter(useBloomPrefilter);
					if (useBloomPrefilter) {
						float bloomPrefilterThreshold = Bloom::getPrefilterThreshold();
						ImGui::DragFloat("Bloom prefilter threshold##pp", &bloomPrefilterThreshold, 0.005f, 0.0f, 100.0f);
						Bloom::setPrefilterThreshold(bloomPrefilterThreshold);
					}
					float bloomBias = Bloom::getBloomBias();
					ImGui::DragFloat("Bloom bias##pp", &bloomBias, 0.005f, 0.0f, 1.0f);
					Bloom::setBloomBias(bloomBias);
				}
			}
			break;
			case 2:
			{
				bool useGaussianBlur = PostProcessing::getUseGaussianBlur();
				if (ImGui::Checkbox("Use gaussian blur##pp", &useGaussianBlur))
					PostProcessing::setUseGaussianBlur(useGaussianBlur);
				if (useGaussianBlur) {
					float blurStrength = GaussianBlur::getStrength();
					ImGui::DragFloat("Blur strength##pp", &blurStrength, 0.005f);
					GaussianBlur::setStrength(blurStrength);
					int blurRadius = GaussianBlur::getRadius();
					ImGui::DragInt("Blur radius##pp", &blurRadius, 0.1f, 0, 50);
					GaussianBlur::setRadius(blurRadius);
					int blurIterations = GaussianBlur::getIterations();
					ImGui::DragInt("Blur iterations##pp", &blurIterations, 0.1f, 0, 50);
					GaussianBlur::setIterations(blurIterations);
					float blurQuality = GaussianBlur::getQuality();
					ImGui::DragFloat("Blur quality##pp", &blurQuality, 0.005f, 0, 1.0f);
					GaussianBlur::setQuality(blurQuality);
				}
			}
			break;
			}
		}
	}
}