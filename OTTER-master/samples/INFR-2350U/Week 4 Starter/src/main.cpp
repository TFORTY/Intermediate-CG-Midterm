//Just a simple handler for simple initialization stuffs
#include "Utilities/BackendHandler.h"

#include <filesystem>
#include <json.hpp>
#include <fstream>

#include <Texture2D.h>
#include <Texture2DData.h>
#include <MeshBuilder.h>
#include <MeshFactory.h>
#include <NotObjLoader.h>
#include <ObjLoader.h>
#include <VertexTypes.h>
#include <ShaderMaterial.h>
#include <RendererComponent.h>
#include <TextureCubeMap.h>
#include <TextureCubeMapData.h>

#include <Timing.h>
#include <GameObjectTag.h>
#include <InputHelpers.h>

#include <IBehaviour.h>
#include <CameraControlBehaviour.h>
#include <FollowPathBehaviour.h>
#include <SimpleMoveBehaviour.h>

int main() {
	int frameIx = 0;
	float fpsBuffer[128];
	float minFps, maxFps, avgFps;

	//Variables for toggles
	bool isTexturesToggled = true;
	int toggleMode = 3;

	BackendHandler::InitAll();

	// Let OpenGL know that we want debug output, and route it to our handler function
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(BackendHandler::GlDebugMessage, nullptr);

	// Enable texturing
	glEnable(GL_TEXTURE_2D);

	// Push another scope so most memory should be freed *before* we exit the app
	{
		#pragma region Shader and ImGui
		Shader::sptr passthroughShader = Shader::Create();
		passthroughShader->LoadShaderPartFromFile("shaders/passthrough_vert.glsl", GL_VERTEX_SHADER);
		passthroughShader->LoadShaderPartFromFile("shaders/passthrough_frag.glsl", GL_FRAGMENT_SHADER);
		passthroughShader->Link();

		// Load our shaders
		Shader::sptr shader = Shader::Create();
		shader->LoadShaderPartFromFile("shaders/vertex_shader.glsl", GL_VERTEX_SHADER);
		shader->LoadShaderPartFromFile("shaders/frag_blinn_phong_textured.glsl", GL_FRAGMENT_SHADER);
		shader->Link();

		glm::vec3 lightPos = glm::vec3(0.0f, 0.0f, 5.0f);
		glm::vec3 lightCol = glm::vec3(1.0f);
		float     lightAmbientPow = 0.09f;
		float     lightSpecularPow = 1.0f;
		glm::vec3 ambientCol = glm::vec3(1.0f);
		float     ambientPow = 0.1f;
		float     lightLinearFalloff = 0.09f;
		float     lightQuadraticFalloff = 0.032f;

		int condition = 0;

		// These are our application / scene level uniforms that don't necessarily update
		// every frame
		shader->SetUniform("u_LightPos", lightPos);
		shader->SetUniform("u_LightCol", lightCol);
		shader->SetUniform("u_AmbientLightStrength", lightAmbientPow);
		shader->SetUniform("u_SpecularLightStrength", lightSpecularPow);
		shader->SetUniform("u_AmbientCol", ambientCol);
		shader->SetUniform("u_AmbientStrength", ambientPow);
		shader->SetUniform("u_LightAttenuationConstant", 1.0f);
		shader->SetUniform("u_LightAttenuationLinear", lightLinearFalloff);
		shader->SetUniform("u_LightAttenuationQuadratic", lightQuadraticFalloff);

		shader->SetUniform("u_Condition", condition);

		PostEffect* basicEffect;

		int activeEffect = 0;
		std::vector<PostEffect*> effects;

		SepiaEffect* sepiaEffect;
		GreyscaleEffect* greyscaleEffect;
		ColorCorrectEffect* colorCorrectEffect;

		BloomEffect* bloomEffect;
		
		// We'll add some ImGui controls to control our shader
		BackendHandler::imGuiCallbacks.push_back([&]() {

			if (ImGui::CollapsingHeader("Toggles"))
			{
				//Toggles on no lighting
				if (ImGui::Button("No Lighting"))
				{
					toggleMode = 0;
					shader->SetUniform("u_Condition", 0); 
					activeEffect = 0;
				}
				//Toggles on ambient lighting only
				if (ImGui::Button("Ambient Only"))
				{
					toggleMode = 1;
					shader->SetUniform("u_Condition", 1);
					activeEffect = 0;
				}
				//Toggles on specular lighting only
				if (ImGui::Button("Specular Only"))
				{
					toggleMode = 2;
					shader->SetUniform("u_Condition", 2);
					activeEffect = 0;
				}
				//Toggles on ambient + specular + diffuse lighitng (DEFAULT) 
				if (ImGui::Button("Ambient + Specular + Diffuse"))
				{
					toggleMode = 3;
					shader->SetUniform("u_Condition", 3);
					activeEffect = 0;
				}
				//Ambient + Specular + Diffuse + Bloom
				if (ImGui::Button("Ambient + Specular + Diffuse + Bloom"))
				{
					toggleMode = 4;
					shader->SetUniform("u_Condition", 3);
					activeEffect = 4;
				}

				//Displays which lighting toggle is on
				ImGui::Text("Lighting Toggle: ", toggleMode);
				if (toggleMode == 0)
				{
					ImGui::SameLine(0.0f, 1.0f);
					ImGui::Text("No Lighting");
				}
				else if (toggleMode == 1)
				{
					ImGui::SameLine(0.0f, 1.0f);
					ImGui::Text("Ambient Lighting");
				}
				else if (toggleMode == 2)
				{
					ImGui::SameLine(0.0f, 1.0f);
					ImGui::Text("Specular Lighting");
				}
				else if (toggleMode == 3)
				{
					ImGui::SameLine(0.0f, 1.0f);
					ImGui::Text("Ambient + Specular + Diffuse");
				}
				else if (toggleMode == 4)
				{
					ImGui::SameLine(0.0f, 1.0f);
					ImGui::Text("Ambient + Specular + Diffuse + Bloom");
				}

				//Toggles textures on/off
				if (ImGui::Button("Toggle Textures"))
				{
					isTexturesToggled = !isTexturesToggled;
				}	

				//Text to clearly display whether textures are on or off
				ImGui::Text("Texture Toggled: ", isTexturesToggled);

				if (isTexturesToggled)
				{
					ImGui::SameLine(0.0f, 1.0f);
					ImGui::Text("ON");
				}
				else
				{
					ImGui::SameLine(0.0f, 1.0f);
					ImGui::Text("OFF");
				}
			}

			if (ImGui::CollapsingHeader("Effect controls"))
			{
				ImGui::SliderInt("Chosen Effect", &activeEffect, 0, effects.size() - 1);

				if (activeEffect == 0)
				{
					ImGui::Text("Active Effect: No Effect");
					PostEffect* temp = (PostEffect*)effects[activeEffect];
				}

				if (activeEffect == 1)
				{
					ImGui::Text("Active Effect: Sepia Effect");

					SepiaEffect* temp = (SepiaEffect*)effects[activeEffect];
					float intensity = temp->GetIntensity();

					if (ImGui::SliderFloat("Intensity", &intensity, 0.0f, 1.0f))
					{
						temp->SetIntensity(intensity);
					}
				}
				if (activeEffect == 2)
				{
					ImGui::Text("Active Effect: Greyscale Effect");
					
					GreyscaleEffect* temp = (GreyscaleEffect*)effects[activeEffect];
					float intensity = temp->GetIntensity();

					if (ImGui::SliderFloat("Intensity", &intensity, 0.0f, 1.0f))
					{
						temp->SetIntensity(intensity);
					}
				}
				if (activeEffect == 3)
				{
					ImGui::Text("Active Effect: Color Correct Effect");

					ColorCorrectEffect* temp = (ColorCorrectEffect*)effects[activeEffect];
					static char input[BUFSIZ];
					ImGui::InputText("Lut File to Use", input, BUFSIZ);

					if (ImGui::Button("SetLUT", ImVec2(200.0f, 40.0f)))
					{
						temp->SetLUT(LUT3D(std::string(input)));
					}
				}
				if (activeEffect == 4)
				{
					ImGui::Text("Active Effect: Bloom Effect");

					BloomEffect* temp = (BloomEffect*)effects[activeEffect];
					float brightnessThreshold = temp->GetThreshold();
					int blurValue = temp->GetPasses();

					if (ImGui::SliderFloat("Brightness Threshold", &brightnessThreshold, 1.0f, 0.0f))
					{
						temp->SetThreshold(brightnessThreshold);
					}
					if (ImGui::SliderInt("Blur Value", &blurValue, 0.0f, 10.f))
					{
						temp->SetPasses(blurValue);
					}
				}
			}

			ImGui::Text("Q/E -> Yaw\nLeft/Right -> Roll\nUp/Down -> Pitch\nY -> Toggle Mode");
		
			minFps = FLT_MAX;
			maxFps = 0;
			avgFps = 0;
			for (int ix = 0; ix < 128; ix++) {
				if (fpsBuffer[ix] < minFps) { minFps = fpsBuffer[ix]; }
				if (fpsBuffer[ix] > maxFps) { maxFps = fpsBuffer[ix]; }
				avgFps += fpsBuffer[ix];
			}
			ImGui::PlotLines("FPS", fpsBuffer, 128);
			ImGui::Text("MIN: %f MAX: %f AVG: %f", minFps, maxFps, avgFps / 128.0f);
			});

		#pragma endregion 

		// GL states
		glEnable(GL_DEPTH_TEST);
		//glEnable(GL_CULL_FACE);
		glDepthFunc(GL_LEQUAL); // New 

		#pragma region TEXTURE LOADING

		// Load some textures from files
		Texture2D::sptr grass = Texture2D::LoadFromFile("images/grass.jpg");
		Texture2D::sptr noSpec = Texture2D::LoadFromFile("images/grassSpec.png");

		Texture2D::sptr house = Texture2D::LoadFromFile("images/houseTex.png");
		Texture2D::sptr barrel = Texture2D::LoadFromFile("images/wood.jpg");
		Texture2D::sptr barrelNormal = Texture2D::LoadFromFile("images/woodNormal.jpg");
		Texture2D::sptr tree = Texture2D::LoadFromFile("images/tree.png");
		Texture2D::sptr straw = Texture2D::LoadFromFile("images/straw.jpg");
		Texture2D::sptr strawBump = Texture2D::LoadFromFile("images/strawBump.jpg");
		Texture2D::sptr horse = Texture2D::LoadFromFile("images/horse.jpg");

		// Load the cube map
		TextureCubeMap::sptr environmentMap = TextureCubeMap::LoadFromImages("images/cubemaps/skybox/ToonSky.jpg"); 

		// Creating an empty texture
		Texture2DDescription desc = Texture2DDescription();  
		desc.Width = 1;
		desc.Height = 1;
		desc.Format = InternalFormat::RGB8;
		Texture2D::sptr texture2 = Texture2D::Create(desc);
		// Clear it with a white colour
		texture2->Clear();

		#pragma endregion

		///////////////////////////////////// Scene Generation //////////////////////////////////////////////////
		#pragma region Scene Generation
		
		// We need to tell our scene system what extra component types we want to support
		GameScene::RegisterComponentType<RendererComponent>();
		GameScene::RegisterComponentType<BehaviourBinding>();
		GameScene::RegisterComponentType<Camera>();

		// Create a scene, and set it to be the active scene in the application
		GameScene::sptr scene = GameScene::Create("test");
		Application::Instance().ActiveScene = scene;

		// We can create a group ahead of time to make iterating on the group faster
		entt::basic_group<entt::entity, entt::exclude_t<>, entt::get_t<Transform>, RendererComponent> renderGroup =
			scene->Registry().group<RendererComponent>(entt::get_t<Transform>());

		// Create a material and set some properties for it
		ShaderMaterial::sptr noTex = ShaderMaterial::Create();
		noTex->Shader = shader;
		noTex->Set("s_Diffuse", texture2);
		noTex->Set("u_Shininess", 2.0f);
		noTex->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr grassMat = ShaderMaterial::Create();
		grassMat->Shader = shader;
		grassMat->Set("s_Diffuse", grass);
		grassMat->Set("s_Specular", noSpec);
		grassMat->Set("u_Shininess", 2.0f);
		grassMat->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr houseMat = ShaderMaterial::Create();
		houseMat->Shader = shader;
		houseMat->Set("s_Diffuse", house);
		houseMat->Set("u_Shininess", 2.0f);
		houseMat->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr barrelMat = ShaderMaterial::Create();
		barrelMat->Shader = shader;
		barrelMat->Set("s_Diffuse", barrel);
		barrelMat->Set("s_Diffuse2", barrelNormal);
		barrelMat->Set("u_Shininess", 2.0f);
		barrelMat->Set("u_TextureMix", 0.25f);

		ShaderMaterial::sptr treeMat = ShaderMaterial::Create();
		treeMat->Shader = shader;
		treeMat->Set("s_Diffuse", tree);
		treeMat->Set("u_Shininess", 2.0f);
		treeMat->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr strawMat = ShaderMaterial::Create();
		strawMat->Shader = shader;
		strawMat->Set("s_Diffuse", straw);
		strawMat->Set("s_Diffuse2", strawBump);
		strawMat->Set("u_Shininess", 2.0f);
		strawMat->Set("u_TextureMix", 0.25f);

		ShaderMaterial::sptr horseMat = ShaderMaterial::Create();
		horseMat->Shader = shader;
		horseMat->Set("s_Diffuse", horse);
		horseMat->Set("u_Shininess", 2.0f);
		horseMat->Set("u_TextureMix", 0.0f);

		//Objects
		GameObject groundObj = scene->CreateEntity("Ground"); 
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/plane.obj");
			groundObj.emplace<RendererComponent>().SetMesh(vao).SetMaterial(grassMat);
		}

		GameObject houseObj = scene->CreateEntity("House");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/house.obj");
			houseObj.emplace<RendererComponent>().SetMesh(vao).SetMaterial(houseMat);
			houseObj.get<Transform>().SetLocalPosition(0.0f, -12.0f, 0.1f);
			houseObj.get<Transform>().SetLocalRotation(90.0f, 0.0f, 180.0f);
		}

		GameObject barrelObj = scene->CreateEntity("Barrel");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/barrel.obj");
			barrelObj.emplace<RendererComponent>().SetMesh(vao).SetMaterial(barrelMat);
			barrelObj.get<Transform>().SetLocalPosition(-9.0f, -8.0f, -0.1f);
			barrelObj.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			barrelObj.get<Transform>().SetLocalScale(glm::vec3(0.2f));
		}

		GameObject barrelObj2 = scene->CreateEntity("Barrel2");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/barrel.obj");
			barrelObj2.emplace<RendererComponent>().SetMesh(vao).SetMaterial(barrelMat);
			barrelObj2.get<Transform>().SetLocalPosition(-12.0f, -6.0f, -0.1f);
			barrelObj2.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			barrelObj2.get<Transform>().SetLocalScale(glm::vec3(0.2f));
		}

		GameObject barrelObj3 = scene->CreateEntity("Barrel3");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/barrel.obj");
			barrelObj3.emplace<RendererComponent>().SetMesh(vao).SetMaterial(barrelMat);
			barrelObj3.get<Transform>().SetLocalPosition(7.0f, -5.0f, -0.1f);
			barrelObj3.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			barrelObj3.get<Transform>().SetLocalScale(glm::vec3(0.2f));
		}

		GameObject barrelObj4 = scene->CreateEntity("Barrel4");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/barrel.obj");
			barrelObj4.emplace<RendererComponent>().SetMesh(vao).SetMaterial(barrelMat);
			barrelObj4.get<Transform>().SetLocalPosition(14.0f, 4.0f, -0.1f);
			barrelObj4.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			barrelObj4.get<Transform>().SetLocalScale(glm::vec3(0.2f));
		}

		GameObject treeObj = scene->CreateEntity("Tree");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/tree.obj");
			treeObj.emplace<RendererComponent>().SetMesh(vao).SetMaterial(treeMat);
			treeObj.get<Transform>().SetLocalPosition(13.0f, -12.0f, 0.45f);
			treeObj.get<Transform>().SetLocalScale(glm::vec3(0.1f));
		}

		GameObject treeObj2 = scene->CreateEntity("Tree2");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/tree.obj");
			treeObj2.emplace<RendererComponent>().SetMesh(vao).SetMaterial(treeMat);
			treeObj2.get<Transform>().SetLocalPosition(-13.0f, -12.0f, 0.45f);
			treeObj2.get<Transform>().SetLocalScale(glm::vec3(0.1f));
		}

		GameObject treeObj3 = scene->CreateEntity("Tree3");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/tree.obj");
			treeObj3.emplace<RendererComponent>().SetMesh(vao).SetMaterial(treeMat);
			treeObj3.get<Transform>().SetLocalPosition(15.0f, -5.0f, 0.45f);
			treeObj3.get<Transform>().SetLocalScale(glm::vec3(0.1f));
		}

		GameObject treeObj4 = scene->CreateEntity("Tree4");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/tree.obj");
			treeObj4.emplace<RendererComponent>().SetMesh(vao).SetMaterial(treeMat);
			treeObj4.get<Transform>().SetLocalPosition(-15.0f, -6.f, 0.45f);
			treeObj4.get<Transform>().SetLocalScale(glm::vec3(0.1f));
		}

		GameObject treeObj5 = scene->CreateEntity("Tree5");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/tree.obj");
			treeObj5.emplace<RendererComponent>().SetMesh(vao).SetMaterial(treeMat);
			treeObj5.get<Transform>().SetLocalPosition(-15.0f, 14.f, 0.45f);
			treeObj5.get<Transform>().SetLocalScale(glm::vec3(0.1f));
		}

		GameObject strawObj = scene->CreateEntity("Straw");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/straw.obj");
			strawObj.emplace<RendererComponent>().SetMesh(vao).SetMaterial(strawMat);
			strawObj.get<Transform>().SetLocalPosition(-12.0f, 3.0f, 0.9f);
			strawObj.get<Transform>().SetLocalRotation(90.0f, 0.0f, 90.0f);
			strawObj.get<Transform>().SetLocalScale(glm::vec3(0.03f));
		}

		GameObject strawObj2 = scene->CreateEntity("Straw2");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/straw.obj");
			strawObj2.emplace<RendererComponent>().SetMesh(vao).SetMaterial(strawMat);
			strawObj2.get<Transform>().SetLocalPosition(-12.0f, 10.0f, 0.9f);
			strawObj2.get<Transform>().SetLocalRotation(90.0f, 0.0f, 90.0f);
			strawObj2.get<Transform>().SetLocalScale(glm::vec3(0.03f));
		}

		GameObject strawObj3 = scene->CreateEntity("Straw3");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/straw.obj");
			strawObj3.emplace<RendererComponent>().SetMesh(vao).SetMaterial(strawMat);
			strawObj3.get<Transform>().SetLocalPosition(9.0f, 3.0f, 0.9f);
			strawObj3.get<Transform>().SetLocalRotation(90.0f, 0.0f, 45.0f);
			strawObj3.get<Transform>().SetLocalScale(glm::vec3(0.03f));
		}

		GameObject horseObj = scene->CreateEntity("Horse"); 
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/horse.obj");
			horseObj.emplace<RendererComponent>().SetMesh(vao).SetMaterial(horseMat);
			horseObj.get<Transform>().SetLocalPosition(13.0f, 0.0f, 0.0f);
			horseObj.get<Transform>().SetLocalRotation(0.0f, 0.0f, 225.0f);
			horseObj.get<Transform>().SetLocalScale(glm::vec3(0.002f));
		}

		GameObject horseObj2 = scene->CreateEntity("Horse2");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/horse.obj");
			horseObj2.emplace<RendererComponent>().SetMesh(vao).SetMaterial(horseMat);
			horseObj2.get<Transform>().SetLocalPosition(-14.0f, 3.0f, 0.0f);
			horseObj2.get<Transform>().SetLocalRotation(0.0f, 0.0f, 90.0f);
			horseObj2.get<Transform>().SetLocalScale(glm::vec3(0.002f));
		}

		GameObject horseObj3 = scene->CreateEntity("Horse3");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/horse.obj");
			horseObj3.emplace<RendererComponent>().SetMesh(vao).SetMaterial(horseMat);
			horseObj3.get<Transform>().SetLocalPosition(-14.0f, 10.0f, 0.0f);
			horseObj3.get<Transform>().SetLocalRotation(0.0f, 0.0f, 90.0f);
			horseObj3.get<Transform>().SetLocalScale(glm::vec3(0.002f));
		}

		GameObject horseObj4 = scene->CreateEntity("Horse4");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/horse.obj");
			horseObj4.emplace<RendererComponent>().SetMesh(vao).SetMaterial(horseMat);
			horseObj4.get<Transform>().SetLocalPosition(14.0f, 14.0f, 0.0f);
			horseObj4.get<Transform>().SetLocalRotation(0.0f, 0.0f, -90.0f);
			horseObj4.get<Transform>().SetLocalScale(glm::vec3(0.002f));

			auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(horseObj4);
			pathing->Points.push_back({ -7.f, 14.f, 0.f });
			pathing->Points.push_back({ 14.f, 14.f, 0.f });
			pathing->Speed = 6.0f;
		}

		GameObject horseObj5 = scene->CreateEntity("Horse5");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/horse.obj");
			horseObj5.emplace<RendererComponent>().SetMesh(vao).SetMaterial(horseMat);
			horseObj5.get<Transform>().SetLocalPosition(4.0f, -3.0f, 0.0f);
			horseObj5.get<Transform>().SetLocalRotation(0.0f, 0.0f, -90.0f);
			horseObj5.get<Transform>().SetLocalScale(glm::vec3(0.002f));

			auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(horseObj5);
			pathing->Points.push_back({ -4.f, -3.f, 0.f });
			pathing->Points.push_back({ -4.f, 6.f, 0.f });
			pathing->Points.push_back({ 4.f, 6.f, 0.f });
			pathing->Points.push_back({ 4.f, -3.f, 0.f });
			pathing->Speed = 6.0f;
		}

		// Create an object to be our camera
		GameObject cameraObject = scene->CreateEntity("Camera");
		{
			cameraObject.get<Transform>().SetLocalPosition(0, 3, 3).LookAt(glm::vec3(0, 0, 0));

			// We'll make our camera a component of the camera object
			Camera& camera = cameraObject.emplace<Camera>();// Camera::Create();
			camera.SetPosition(glm::vec3(0, 3, 3));
			camera.SetUp(glm::vec3(0, 0, 1));
			camera.LookAt(glm::vec3(0));
			camera.SetFovDegrees(90.0f); // Set an initial FOV
			camera.SetOrthoHeight(3.0f);
			BehaviourBinding::Bind<CameraControlBehaviour>(cameraObject);
		}

		//Post-Processing Effects
		int width, height;
		glfwGetWindowSize(BackendHandler::window, &width, &height);

		GameObject framebufferObject = scene->CreateEntity("Basic Effect");
		{
			basicEffect = &framebufferObject.emplace<PostEffect>();
			basicEffect->Init(width, height);
		}
		effects.push_back(basicEffect);

		GameObject sepiaEffectObject = scene->CreateEntity("Sepia Effect");
		{
			sepiaEffect = &sepiaEffectObject.emplace<SepiaEffect>();
			sepiaEffect->Init(width, height);
		}
		effects.push_back(sepiaEffect); 

		GameObject greyscaleEffectObject = scene->CreateEntity("Greyscale Effect");
		{
			greyscaleEffect = &greyscaleEffectObject.emplace<GreyscaleEffect>();
			greyscaleEffect->Init(width, height);
		}
		effects.push_back(greyscaleEffect);
		
		GameObject colorCorrectEffectObject = scene->CreateEntity("Greyscale Effect");
		{
			colorCorrectEffect = &colorCorrectEffectObject.emplace<ColorCorrectEffect>();
			colorCorrectEffect->Init(width, height);
		}
		effects.push_back(colorCorrectEffect);

		GameObject bloomEffectObject = scene->CreateEntity("Bloom Effect");
		{
			bloomEffect = &bloomEffectObject.emplace<BloomEffect>();
			bloomEffect->Init(width, height);
		}
		effects.push_back(bloomEffect);

		#pragma endregion 
		//////////////////////////////////////////////////////////////////////////////////////////

		/////////////////////////////////// SKYBOX ///////////////////////////////////////////////
		{
			// Load our shaders
			Shader::sptr skybox = std::make_shared<Shader>();
			skybox->LoadShaderPartFromFile("shaders/skybox-shader.vert.glsl", GL_VERTEX_SHADER);
			skybox->LoadShaderPartFromFile("shaders/skybox-shader.frag.glsl", GL_FRAGMENT_SHADER);
			skybox->Link();

			ShaderMaterial::sptr skyboxMat = ShaderMaterial::Create();
			skyboxMat->Shader = skybox;  
			skyboxMat->Set("s_Environment", environmentMap);
			skyboxMat->Set("u_EnvironmentRotation", glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0))));
			skyboxMat->RenderLayer = 100;

			MeshBuilder<VertexPosNormTexCol> mesh;
			MeshFactory::AddIcoSphere(mesh, glm::vec3(0.0f), 1.0f);
			MeshFactory::InvertFaces(mesh);
			VertexArrayObject::sptr meshVao = mesh.Bake();
			
			GameObject skyboxObj = scene->CreateEntity("skybox");  
			skyboxObj.get<Transform>().SetLocalPosition(0.0f, 0.0f, 0.0f);
			skyboxObj.get_or_emplace<RendererComponent>().SetMesh(meshVao).SetMaterial(skyboxMat);
		}
		////////////////////////////////////////////////////////////////////////////////////////


		// We'll use a vector to store all our key press events for now (this should probably be a behaviour eventually)
		std::vector<KeyPressWatcher> keyToggles;
		{
			// This is an example of a key press handling helper. Look at InputHelpers.h an .cpp to see
			// how this is implemented. Note that the ampersand here is capturing the variables within
			// the scope. If you wanted to do some method on the class, your best bet would be to give it a method and
			// use std::bind
			keyToggles.emplace_back(GLFW_KEY_T, [&]() { cameraObject.get<Camera>().ToggleOrtho(); });
		}

		// Initialize our timing instance and grab a reference for our use
		Timing& time = Timing::Instance();
		time.LastFrame = glfwGetTime();

		///// Game loop /////
		while (!glfwWindowShouldClose(BackendHandler::window)) {
			glfwPollEvents();

			// Update the timing
			time.CurrentFrame = glfwGetTime();
			time.DeltaTime = static_cast<float>(time.CurrentFrame - time.LastFrame);

			time.DeltaTime = time.DeltaTime > 1.0f ? 1.0f : time.DeltaTime;

			// Update our FPS tracker data
			fpsBuffer[frameIx] = 1.0f / time.DeltaTime;
			frameIx++;
			if (frameIx >= 128)
				frameIx = 0;

			// We'll make sure our UI isn't focused before we start handling input for our game
			if (!ImGui::IsAnyWindowFocused()) {
				// We need to poll our key watchers so they can do their logic with the GLFW state
				// Note that since we want to make sure we don't copy our key handlers, we need a const
				// reference!
				for (const KeyPressWatcher& watcher : keyToggles) {
					watcher.Poll(BackendHandler::window);
				}
			}

			//Changes the diffuse material to be no texture
			if (!isTexturesToggled)
			{
				grassMat->Set("s_Diffuse", texture2);
				houseMat->Set("s_Diffuse", texture2);
				barrelMat->Set("s_Diffuse", texture2);
				treeMat->Set("s_Diffuse", texture2);
				strawMat->Set("s_Diffuse", texture2);		
				horseMat->Set("s_Diffuse", texture2);		
			}
			//Returns the difuse material to its original texture
			else
			{
				grassMat->Set("s_Diffuse", grass);
				houseMat->Set("s_Diffuse", house);
				barrelMat->Set("s_Diffuse", barrel);
				treeMat->Set("s_Diffuse", tree);
				strawMat->Set("s_Diffuse", straw);
				horseMat->Set("s_Diffuse", horse);
			}

			// Iterate over all the behaviour binding components
			scene->Registry().view<BehaviourBinding>().each([&](entt::entity entity, BehaviourBinding& binding) {
				// Iterate over all the behaviour scripts attached to the entity, and update them in sequence (if enabled)
				for (const auto& behaviour : binding.Behaviours) {
					if (behaviour->Enabled) {
						behaviour->Update(entt::handle(scene->Registry(), entity));
					}
				}
			});

			// Clear the screen
			basicEffect->Clear();
			for (int i = 0; i < effects.size(); i++)
			{
				effects[i]->Clear();
			}

			glClearColor(0.08f, 0.17f, 0.31f, 1.0f);
			glEnable(GL_DEPTH_TEST);
			glClearDepth(1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// Update all world matrices for this frame
			scene->Registry().view<Transform>().each([](entt::entity entity, Transform& t) {
				t.UpdateWorldMatrix();
			});
			
			// Grab out camera info from the camera object
			Transform& camTransform = cameraObject.get<Transform>();
			glm::mat4 view = glm::inverse(camTransform.LocalTransform());
			glm::mat4 projection = cameraObject.get<Camera>().GetProjection();
			glm::mat4 viewProjection = projection * view;

			//Rotates the horse when moving
			if (horseObj4.get<Transform>().GetLocalPosition().x <= -6.9f)
			{
				horseObj4.get<Transform>().SetLocalRotation(glm::vec3(0.f, 0.f, 90.f));
			}
			else if (horseObj4.get<Transform>().GetLocalPosition().x >= 13.9f)
			{
				horseObj4.get<Transform>().SetLocalRotation(glm::vec3(0.f, 0.f, -90.f));
			}

			if (horseObj5.get<Transform>().GetLocalPosition().x <= -3.9f)
			{
				horseObj5.get<Transform>().SetLocalRotation(glm::vec3(0.f, 0.f, 180.f));
			}
			else if (horseObj5.get<Transform>().GetLocalPosition().y >= 5.9f)
			{
				horseObj5.get<Transform>().SetLocalRotation(glm::vec3(0.f, 0.f, 90.f));
			}
			else if (horseObj5.get<Transform>().GetLocalPosition().x >= 3.9f)
			{
				horseObj5.get<Transform>().SetLocalRotation(glm::vec3(0.f, 0.f, 0.f));
			}
			else
			{
				horseObj5.get<Transform>().SetLocalRotation(glm::vec3(0.f, 0.f, -90.f));
			}
	
			// Sort the renderers by shader and material, we will go for a minimizing context switches approach here,
			// but you could for instance sort front to back to optimize for fill rate if you have intensive fragment shaders
			renderGroup.sort<RendererComponent>([](const RendererComponent& l, const RendererComponent& r) {
				// Sort by render layer first, higher numbers get drawn last
				if (l.Material->RenderLayer < r.Material->RenderLayer) return true;
				if (l.Material->RenderLayer > r.Material->RenderLayer) return false;

				// Sort by shader pointer next (so materials using the same shader run sequentially where possible)
				if (l.Material->Shader < r.Material->Shader) return true;
				if (l.Material->Shader > r.Material->Shader) return false;

				// Sort by material pointer last (so we can minimize switching between materials)
				if (l.Material < r.Material) return true;
				if (l.Material > r.Material) return false;
				
				return false;
			});

			// Start by assuming no shader or material is applied
			Shader::sptr current = nullptr;
			ShaderMaterial::sptr currentMat = nullptr;

			basicEffect->BindBuffer(0);

			// Iterate over the render group components and draw them
			renderGroup.each( [&](entt::entity e, RendererComponent& renderer, Transform& transform) {
				// If the shader has changed, set up it's uniforms
				if (current != renderer.Material->Shader) {
					current = renderer.Material->Shader;
					current->Bind();
					BackendHandler::SetupShaderForFrame(current, view, projection);
				}
				// If the material has changed, apply it
				if (currentMat != renderer.Material) {
					currentMat = renderer.Material;
					currentMat->Apply();
				}
				// Render the mesh
				BackendHandler::RenderVAO(renderer.Material->Shader, renderer.Mesh, viewProjection, transform);
			});

			basicEffect->UnbindBuffer();

			effects[activeEffect]->ApplyEffect(basicEffect);
			
			effects[activeEffect]->DrawToScreen();
		
			// Draw our ImGui content
			BackendHandler::RenderImGui();

			scene->Poll();
			glfwSwapBuffers(BackendHandler::window);
			time.LastFrame = time.CurrentFrame;
		}

		// Nullify scene so that we can release references
		Application::Instance().ActiveScene = nullptr;
		BackendHandler::ShutdownImGui();
	}	

	// Clean up the toolkit logger so we don't leak memory
	Logger::Uninitialize();
	return 0;
}