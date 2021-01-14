#include "starter_app.h"
#include <system/platform.h>
#include <graphics/sprite_renderer.h>
#include <graphics/texture.h>
#include <graphics/mesh.h>
#include <graphics/primitive.h>
#include <assets/png_loader.h>
#include <graphics/image_data.h>
#include <graphics/font.h>
#include <graphics/scene.h>
#include <maths/vector2.h>
#include <input/input_manager.h>
#include <input/sony_controller_input_manager.h>
#include <input/keyboard.h>
#include <maths/math_utils.h>
#include <graphics/renderer_3d.h>

#include "VirtualSystem.h"
#include "VirtualSonySample.h"
#include <vector>

StarterApp::StarterApp(gef::Platform& platform) :
	Application(platform),
	sprite_renderer_(NULL),
	font_(NULL),
	renderer_3d_(NULL)
{
	levelX = 0.625f;
	levelY= 0.0f;
	levelZ = 3.0f;
	levelScale = 0.00875f;
	rotX = 0.0f;
	rotY = 0.0f;
	rotZ = 0.0f;
}

bool  StarterApp::sampleIsMarkerFound ( int idx )
{
	return virtualSystem_->IsMarkerFound(idx);
}

void StarterApp::sampleGetTransform ( int idx, gef::Matrix44* mat )
{
	virtualSystem_->GetMarkerTransform ( idx, mat );
}

void StarterApp::Init()
{
	sprite_renderer_ = gef::SpriteRenderer::Create(platform_);
	renderer_3d_ = gef::Renderer3D::Create(platform_);
	input_manager_ = gef::InputManager::Create(platform_);
	primitive_builder_ = new PrimitiveBuilder ( platform_ );

	InitFont();
	SetupCamera();
	SetupLights();

	loadScenes();

	virtualSystem_ = VirtualSystem::Create ();
	virtualSystem_->Init ( primitive_builder_ );

	////////////////////////////////////////////////////////////
	// GAME LOGIC //////////////

	testObject_ = new GameObject;
	testObject_->position_ = gef::Vector4 ( 0.0f, 0.0f, 0.0f );
	//testObject_->scale_ = gef::Vector4 ( 0.05f, 0.05f, 0.05f );
	testObject_->scale_ = gef::Vector4 ( 1.0f, 1.0f, 1.0f );
	//testObject_->set_mesh ( primitive_builder_->GetDefaultCubeMesh () );
	testObject_->set_mesh ( primitive_builder_->CreateBoxMesh ( gef::Vector4 ( 0.059f, 0.059f, 0.001f ), gef::Vector4 ( 0.0f, 0.0f, 0.0f ) ) );

	//////////////////////////////////////////////////////////
	// initialise sony framework
	sampleInitialize ();
	smartInitialize ();

	// reset marker tracking
	AppData* dat = sampleUpdateBegin ();
	smartTrackingReset ();
	sampleUpdateEnd ( dat );

	///////////////////////////////////////////////////////

	initPLevel();

}


void StarterApp::CleanUp()
{
	//////////////////////////////////////////////////////////
	smartRelease ();
	sampleRelease ();

	////////////////////////////////////////////////////////////
	// GAME LOGIC //////////////
	delete testObject_;
	testObject_ = NULL;


	//////////////////////////////////////////////////////////

	virtualSystem_->CleanUp ();
	delete virtualSystem_;
	virtualSystem_ = NULL;

	delete primitive_builder_;
	primitive_builder_ = NULL;

	CleanUpFont();

	delete input_manager_;
	input_manager_ = NULL;

	delete sprite_renderer_;
	sprite_renderer_ = NULL;

	delete renderer_3d_;
	renderer_3d_ = NULL;

	delete pLevel;
	pLevel = NULL;

}

bool StarterApp::Update(float frame_time)
{
	fps_ = 1.0f / frame_time;
	keyW = false;
	destroyButtonDown = false;
	protectButtonDown = false;
	//pLevel->setLevelCenter(gef::Vector4(levelX, levelY, levelZ), primitive_builder_);

	// read input devices
	if (input_manager_)
	{
		input_manager_->Update ();

		// controller input
		gef::SonyControllerInputManager* controller_manager = input_manager_->controller_input ();
		if (controller_manager)
		{
		}

		// keyboard input
		gef::Keyboard* keyboard = input_manager_->keyboard ();
		virtualSystem_->ProcessKeyboardInput ( keyboard, frame_time );
	}

	virtualSystem_->Update ( frame_time );

	//////////////////////////////////////////////////////////
	AppData* dat = sampleUpdateBegin ();

	// use the tracking library to try and find markers
	smartUpdate ( dat->currentImage );
	sampleUpdateEnd ( dat );
		
	////////////////////////////////////////////////////////////
	// GAME LOGIC //////////////

	//// check to see if a particular marker can be found
	//if (sampleIsMarkerFound ( 0 ))
	//{
	//	// marker is being tracked, get its transform
	//	gef::Matrix44 marker_transform;
	//	sampleGetTransform (
	//		0,
	//		&marker_transform );

	//	// set the transform of the 3D mesh instance to draw on
	//	// top of the marker
	//	gef::Matrix44 trans = marker_transform;

	//	gef::Matrix44 scale;
	//	scale.Scale ( gef::Vector4 (0.1f, 0.1f, 0.1f ) );
	//	gef::Vector4 position = trans.GetTranslation ();
	//	position.set_y ( position.y () + 0.15f );
	//	trans = scale * trans;
	//	trans.SetTranslation ( position );


	//	testObject_->set_transform ( trans );

	//}

	for (int i = 0; i < 6; i++)
	{
		if (sampleIsMarkerFound ( i ))
		{
			// marker is being tracked, get its transform
			gef::Matrix44 marker_transform;
			sampleGetTransform ( i, &marker_transform );
			// set the transform of the 3D mesh instance to draw on
			// top of the marker
			gef::Vector4 position = marker_transform.GetTranslation ();
			position.set_y ( position.y () + 0.10f );
			marker_transform.SetTranslation ( position );

			levelX = marker_transform.GetTranslation().x();
			levelY = marker_transform.GetTranslation().y();
			levelZ = marker_transform.GetTranslation().z();
			levelScale = marker_transform.GetScale().x();

			testObject_->set_transform ( marker_transform );
			pLevel->setLevelCenter2(marker_transform, primitive_builder_, gef::Vector4(gef::DegToRad(rotX), gef::DegToRad(rotY), gef::DegToRad(rotZ)));
			
		}
	}

	//pLevel->setLevelCenter(gef::Vector4(levelX, levelY, -levelZ), primitive_builder_);
	//pLevel->setScale(levelScale, primitive_builder_);

	//////////////////////////////////////////////////////////

	return true;
}

void StarterApp::Render()
{
	//////////////////////////////////////////////////////////
	AppData* dat = sampleRenderBegin ();

	gef::Matrix44 projection_matrix;

	projection_matrix = platform_.PerspectiveProjectionFov(camera_fov_, (float)platform_.width() / (float)platform_.height(), near_plane_, far_plane_);
	renderer_3d_->set_projection_matrix(projection_matrix);

	// draw meshes here
	renderer_3d_->Begin();
	virtualSystem_->Render ( renderer_3d_ );

	gef::Matrix44 view_matrix;
	view_matrix.SetIdentity ();
	renderer_3d_->set_view_matrix ( view_matrix );

	////////////////////////////////////////////////////////////
	// GAME LOGIC //////////////


	renderer_3d_->DrawMesh ( *testObject_ );

	pLevel->renderLevel(renderer_3d_);

	////////////////////////////////////////////////////////////


	renderer_3d_->End();

	// setup the sprite renderer, but don't clear the frame buffer
	// draw 2D sprites here
	sprite_renderer_->Begin(false);
	DrawHUD();
	sprite_renderer_->End();



	sampleRenderEnd ();
	//////////////////////////////////////////////////////////
}
void StarterApp::InitFont()
{
	font_ = new gef::Font(platform_);
	font_->Load("comic_sans");
}

void StarterApp::CleanUpFont()
{
	delete font_;
	font_ = NULL;
}

void StarterApp::DrawHUD()
{
	if(font_)
	{
		// display frame rate
		font_->RenderText(sprite_renderer_, gef::Vector4(850.0f, 510.0f, -0.9f), 1.0f, 0xffffffff, gef::TJ_LEFT, "FPS: %.1f", fps_);
		
	}

	// Start the Dear ImGui frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	//Create ImGui Windows
	ImGui::Begin("Test");

	ImGui::DragFloat("LevelX", &levelX, 0.125f, -5.0f, 5.0f, "%.2f");
	ImGui::DragFloat("LevelY", &levelY, 0.125f, -5.0f, 5.0f, "%.2f");
	ImGui::DragFloat("LevelZ", &levelZ, 0.125f, -5.0f, 5.0f, "%.2f");
	ImGui::DragFloat("LevelScale", &levelScale, 0.0625f / (2.0f * 100.0f), 0.0000000000000000000001f, 0.01f, "%.8f");


	ImGui::DragFloat("RotX", &rotX, 0.5f, 0.0f, 360.0f, "%.2f");
	ImGui::DragFloat("RotY", &rotY, 0.5f, 0.0f, 360.0f, "%.2f");
	ImGui::DragFloat("RotZ", &rotZ, 0.5f, 0.0f, 360.0f, "%.2f");

	ImGui::DragFloat("Spacing", &picrossSpacing, 0.5f, 0.0f, 10.0f, "%.2f");

	ImGui::Text("RayDirection: (%f, %f, %f, %F)", rayDirValues.x(), rayDirValues.y(), rayDirValues.z(), rayDirValues.w(), "%.2f");

	std::string axesLabels[3] = { "X", "Y", "Z" };

	for (int i = 0; i < 3; ++i)
	{
		std::string label = "";
		label.append(axesLabels[i]);
		label.append(" Axis");
		if (ImGui::InputInt(label.c_str(), &pushingControls[i].first))
		{
			pLevel->pushIntoLevel(i, pushingControls[i].second, pushingControls[i].first);
			pLevel->updateNumbers(numNumbers, numberScenes, camera_eye_);
		}
		if (ImGui::Button("Reverse Direction"))
		{
			pushingControls[i].second = !pushingControls[i].second;
		}
	}
	ImGui::End();


	//Assemble Together Draw Data
	ImGui::Render();
	//Render Draw Data
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void StarterApp::SetupLights()
{
	gef::PointLight default_point_light;
	default_point_light.set_colour(gef::Colour(0.7f, 0.7f, 1.0f, 1.0f));
	default_point_light.set_position(gef::Vector4(-300.0f, -500.0f, 100.0f));

	gef::Default3DShaderData& default_shader_data = renderer_3d_->default_shader_data();
	default_shader_data.set_ambient_light_colour(gef::Colour(0.5f, 0.5f, 0.5f, 1.0f));
	default_shader_data.AddPointLight(default_point_light);
}

void StarterApp::SetupCamera()
{
	// initialise the camera settings
	bool newCameraPos = true;
	newCameraPos ? camera_eye_ = gef::Vector4(5.0f, 5.0f, -350.0f) :	camera_eye_ = gef::Vector4(5.0f, 5.0f, 215.0f);
	camera_lookat_ = gef::Vector4(0.0f, 0.0f, 0.0f);
	camera_up_ = gef::Vector4(0.0f, 1.0f, 0.0f);
	camera_fov_ = gef::DegToRad(45.0f);
	near_plane_ = 0.01f;
	far_plane_ = 100.f;
}

void StarterApp::UpdateCamera ()
{
	// initialise the camera settings
	camera_eye_ = gef::Vector4 ( 1.5f, 1.0f, 1.5f );
	camera_lookat_ = gef::Vector4 ( 0.0f, 0.0f, 0.0f );
	camera_up_ = gef::Vector4 ( 0.0f, 1.0f, 0.0f );
	camera_fov_ = gef::DegToRad ( 45.0f );
	near_plane_ = 0.01f;
	far_plane_ = 1000.f;
}

void StarterApp::loadScenes()
{

	//Number Scenes
	gef::Matrix44 finalTransform = gef::Matrix44::kIdentity;
	gef::Matrix44 rotMatrix = gef::Matrix44::kIdentity;
	gef::Matrix44 scaleMatrix = gef::Matrix44::kIdentity;
	gef::Matrix44 transformMatrix = gef::Matrix44::kIdentity;
	float scaleF = 10.0f;
	float rotF = 90.0f;
	rotMatrix.RotationZ(gef::DegToRad(rotF));
	scaleMatrix.Scale(gef::Vector4(scaleF, scaleF, scaleF, 1.0f));
	transformMatrix.SetTranslation(gef::Vector4(100.0f, 100.0f, 0.0f));
	finalTransform = rotMatrix * scaleMatrix * transformMatrix;

	for (int i = 0; i < numNumbers; ++i)
	{
		numberScenes[i].first = nullptr;
		numberScenes[i].second = nullptr;

		numberScenes[i].first = new gef::Scene();
		numberScenes[i].second = new gef::MeshInstance();
		std::string numFilepath = "number";
		numFilepath.append(std::to_string(i + 1));
		numFilepath.append(".scn");

		numberScenes[i].first->ReadSceneFromFile(platform_, numFilepath.c_str());
		numberScenes[i].first->CreateMaterials(platform_);
		numberScenes[i].second->set_mesh(GetFirstMesh(numberScenes[i].first));

		numberScenes[i].second->set_transform(finalTransform);
	}
}

void StarterApp::initPLevel()
{
	pLevel = new PicrossLevel(primitive_builder_, platform_, Picross::getLayout1());
	pLevel->setCameraPosPtr(&camera_eye_);
	picrossSpacing = 0.0f;


	pLevel->setSpacing(picrossSpacing);
	pLevel->updateNumbers(numNumbers, &numberScenes[0], camera_eye_);
	keyW = false;
	//pLevel->pushIntoLevel(false, true, false, false, false, true, 2);
	for (int i = 0; i < 3; ++i)
	{
		pushingControls[i] = std::pair<int, bool>(0, false);
	}

	destroyButtonDown = false;
	protectButtonDown = false;
	destroyKey = gef::Keyboard::KC_R;
	protectKey = gef::Keyboard::KC_P;
	cameraDist = 350.0f;
	cameraYOffset = 5.0f;
	cameraXZOffset = 5.0f;
	cameraRotAmount = 180.0f;
}

gef::Mesh* StarterApp::GetFirstMesh ( gef::Scene* scene )
{
	gef::Mesh* mesh = NULL;

	if (scene)
	{
		// now check to see if there is any mesh data in the file, if so lets create a mesh from it
		if (scene->mesh_data.size () > 0)
			mesh = scene->CreateMesh ( platform_, scene->mesh_data.front () );
	}

	return mesh;
}


void StarterApp::ReadSceneAndAssignFirstMesh ( const char* filename, gef::Scene** scene, gef::Mesh** mesh )
{
	gef::Scene* scn = new gef::Scene;
	scn->ReadSceneFromFile ( platform_, filename );

	// we do want to render the data stored in the scene file so lets create the materials from the material data present in the scene file
	scn->CreateMaterials ( platform_ );

	*mesh = GetFirstMesh ( scn );
	*scene = scn;
}

bool StarterApp::IsColliding_SphereToSphere ( const gef::MeshInstance& meshInstance1, const gef::MeshInstance& meshInstance2 )
{
	gef::Sphere sphere1 = meshInstance1.mesh ()->bounding_sphere ();
	gef::Sphere sphere2 = meshInstance2.mesh ()->bounding_sphere ();

	gef::Sphere sphere1_transformed = sphere1.Transform ( meshInstance1.transform () );
	gef::Sphere sphere2_transformed = sphere2.Transform ( meshInstance2.transform () );

	gef::Vector4 offset = sphere1_transformed.position () - sphere2_transformed.position ();
	float distance = sqrtf(offset.x() * offset.x() + offset.y() * offset.y() + offset.z() * offset.z());
	float combined_radii = sphere1_transformed.radius () + sphere2_transformed.radius ();

	return distance < combined_radii;
}


bool StarterApp::IsColliding_AABBToAABB ( const gef::MeshInstance& meshInstance1, const gef::MeshInstance& meshInstance2 )
{
	gef::Aabb Aabb1 = meshInstance1.mesh ()->aabb ();
	gef::Aabb Aabb2 = meshInstance2.mesh ()->aabb ();

	gef::Aabb Aabb1_t = Aabb1.Transform ( meshInstance1.transform () );
	gef::Aabb Aabb2_t = Aabb2.Transform ( meshInstance2.transform () );

	if (Aabb1_t.max_vtx ().x () > Aabb2_t.min_vtx ().x () &&
		Aabb1_t.min_vtx ().x () < Aabb2_t.max_vtx ().x () &&
		Aabb1_t.max_vtx ().y () > Aabb2_t.min_vtx ().y () &&
		Aabb1_t.min_vtx ().y () < Aabb2_t.max_vtx ().y () &&
		Aabb1_t.max_vtx ().z () > Aabb2_t.min_vtx ().z () &&
		Aabb1_t.min_vtx ().z () < Aabb2_t.max_vtx ().z ()
		)
	{
		return true;
	}

	return false;

}