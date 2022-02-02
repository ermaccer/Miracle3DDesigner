#include "M3DManager.h"
#include "Miracle3D.h"
#include "..\core\App.h"
#include <CommCtrl.h>
#include <shlobj.h>
#include <WinUser.h>
#include <filesystem>
#include <windowsx.h>
#include <cfloat>
#include <float.h>
#include "..\core\IniReader.h"
#include "..\core\FileFunctions.h"
#include "..\resource.h"

void M3DManager::Init(HWND * list, HWND * box, HWND * key, HWND * table)
{
	eApp::bIsReady = false;
	ModelType = 0;
	WorkType = 0;
	hGroupBox = box;
	hTextureNameBox = key;
	hFileNameBox = table;
	hLogBox = list;
}

void M3DManager::OpenFile(std::wstring input)
{

	Skeleton.clear();
	Models.clear();
	Sections.clear();
	InputPath = L" ";
	OutputPath = L" ";

	InputPath = input;

	SetWindowText(*hTextureNameBox, L"");
	SetWindowText(*hFileNameBox, L"");
	SetWindowText(*hLogBox, L"");
	SendMessage(*hGroupBox, CB_RESETCONTENT, 0, 0);

	ReadFile();
}

void M3DManager::OpenINI(std::wstring input)
{
	Skeleton.clear();
	Models.clear();
	Sections.clear();
	InputPath = L" ";
	OutputPath = L" ";

	InputPath = input;

	SetWindowText(*hTextureNameBox, L"");
	SetWindowText(*hFileNameBox, L"");
	SetWindowText(*hLogBox, L"");
	SendMessage(*hGroupBox, CB_RESETCONTENT, 0, 0);

	ReadINI();
}

void M3DManager::ReadFile()
{
	if (InputPath.empty())
		return;
	pFile.open(InputPath, std::ifstream::binary);

	if (!pFile.is_open())
	{
		MessageBox(eApp::hWindow, L"Failed to open file!", L"Error", MB_ICONERROR);
		return;
	}

	if (pFile.is_open())
	{
		Log(L"Opening: " + wsplitString(InputPath, true));
		m3d_header m3d;
		pFile.read((char*)&m3d, sizeof(m3d_header));

		if (!(strncmp(m3d.header,"ecbnt,t",7) == 0))
		{
			MessageBox(eApp::hWindow, L"Input file is not a Miracle3D model!", L"Error", MB_ICONERROR);
			return;
		}

		if (!IsValidM3DFile(m3d.type))
		{
			MessageBox(eApp::hWindow, L"Input file is not supported Miracle3D model!", L"Error", MB_ICONERROR);
			return;
		}


		if (!(m3d.version == 1))
		{
			MessageBox(eApp::hWindow, L"Input file is using not supported version of Miracle3D model!", L"Error", MB_ICONERROR);
			return;
		}

		ModelType = m3d.type;
		std::wstring message = L"Model Type: ";
		message += GetModelType(ModelType);
		Log(message);

		if (m3d.type == M3D_MODEL_ANIMATED)
		{
			for (int i = 0; i < m3d.type - 1; i++)
			{
				m3d_section section;
				pFile.read((char*)&section, sizeof(m3d_section));
				Sections.push_back(section);
			}

			int endInfo[3];
			pFile.read((char*)&endInfo, sizeof(endInfo));

			for (int i = 0; i < Sections.size(); i++)
			{	
				if (Sections[i].type == MODEL_INFO_IDENT)
				{
					pFile.seekg(Sections[i].offset, pFile.beg);
					model_info info;
					pFile.read((char*)&info, sizeof(model_info));
					ModelInfo = info;

					Log(L"Bones: " + std::to_wstring(ModelInfo.boneAmount));
					Log(L"Objects: " + std::to_wstring(ModelInfo.normalObjects));
					Log(L"Skinned Objects: " + std::to_wstring(ModelInfo.skinnedObjects));
					Log(L"Dummies: " + std::to_wstring(ModelInfo.dummyObjects));
					std::string str(ModelInfo.textureName, strlen(ModelInfo.textureName));
					std::wstring str2(str.length(), L' ');
					std::copy(str.begin(), str.end(), str2.begin());
					SetTextureText(str2);
				}
				if (Sections[i].type == GROUP_INFO_IDENT)
				{
					pFile.seekg(Sections[i].offset, pFile.beg);
					int amount = ModelInfo.dummyObjects + ModelInfo.normalObjects + ModelInfo.skinnedObjects;
					std::vector<v_data> tmpVerts;
					std::vector<uv> tmpMaps;
					std::vector<vn> tmpNormals;
					std::vector<face> tmpFaces;
					std::vector<bone_link> tmpLinks;

					for (int a = 0; a < amount; a++)
					{
						Miracle3DModel model;
						group_info grp;
						pFile.read((char*)&grp, sizeof(group_info));
						model.Name = grp.name;
						model.type = grp.type;
						if (grp.type == GROUP_STATIC || grp.type == GROUP_DUMMY)
						{
							for (int x = 0; x < grp.verts; x++)
							{
								v_data vdat;
								v vert;
								pFile.read((char*)&vert, sizeof(v));
								vdat.vert = vert;
								vn norm;
								pFile.read((char*)&norm, sizeof(vn));
								uv map;
								pFile.read((char*)&map, sizeof(uv));

								tmpVerts.push_back(vdat);
								tmpMaps.push_back(map);
								tmpNormals.push_back(norm);
				
							}
							for (int x = 0; x < grp.faces; x++)
							{
								face f;
								pFile.read((char*)&f, sizeof(face));
								tmpFaces.push_back(f);

							}
							model.Faces = tmpFaces;
							model.Verts = tmpVerts;
							model.Normals = tmpNormals;
							model.Maps = tmpMaps;
							tmpFaces.clear();
							tmpVerts.clear();
							tmpNormals.clear();
							tmpMaps.clear();

							Models.push_back(model);
						
					    }
						else if (grp.type == GROUP_ANIMATED)
						{
		
							for (int x = 0; x < grp.verts; x++)
							{
								v_data vdat;
								v vert;
								pFile.read((char*)&vert, sizeof(v));
								vdat.vert = vert;
								vn norm;
								pFile.read((char*)&norm, sizeof(vn));
								uv map;
								pFile.read((char*)&map, sizeof(uv));

								short boneLinks;
								
								pFile.read((char*)&boneLinks, sizeof(short));
								if (boneLinks > 0)
								{

									for (int c = 0; c < boneLinks; c++)
									{
										bone_link link;
										pFile.read((char*)&link, sizeof(bone_link));
										tmpLinks.push_back(link);
									}

								}
								vdat.BoneLinks = tmpLinks;
								tmpVerts.push_back(vdat);
								tmpMaps.push_back(map);
								tmpNormals.push_back(norm);
						
								tmpLinks.clear();

							}
							for (int x = 0; x < grp.faces; x++)
							{
								face f;
								pFile.read((char*)&f, sizeof(face));
								tmpFaces.push_back(f);

							}
							model.Faces = tmpFaces;
							model.Verts = tmpVerts;
							model.Normals = tmpNormals;
							model.Maps = tmpMaps;
							tmpFaces.clear();
							tmpVerts.clear();
							tmpNormals.clear();
							tmpMaps.clear();

							Models.push_back(model);
						}

						std::string str(grp.name, strlen(grp.name));
						std::wstring str2(str.length(), L' ');
						std::copy(str.begin(), str.end(), str2.begin());
						wchar_t temp[256];
						wsprintf(temp, L"Group (%s): Faces - %d Verts - %d", str2.c_str(), grp.faces, grp.verts);
						Log(temp);
						ComboBox_AddString(*hGroupBox, str2.c_str());
				
					}
				}
				if (Sections[i].type == BONE_INFO_IDENT)
				{
					pFile.seekg(Sections[i].offset, pFile.beg);
					for (int a = 0; a < ModelInfo.boneAmount; a++)
					{
						bone_info bone;
						pFile.read((char*)&bone, sizeof(bone_info));
						Skeleton.push_back(bone);
						std::string str(bone.boneName, strlen(bone.boneName));
						std::wstring str2(str.length(), L' ');
						std::copy(str.begin(), str.end(), str2.begin());
						wchar_t temp[256];
						wsprintf(temp, L"Bone (%s): ID - %d Parent - %d", str2.c_str(), bone.boneID, bone.parentID);
						Log(temp);
					}
				}
			}
		}
		else if (m3d.type == M3D_MODEL_STATIC)
		{
		    for (int i = 0; i < m3d.type - 1; i++)
		    {
		    	m3d_section section;
		    	pFile.read((char*)&section, sizeof(m3d_section));
		    	Sections.push_back(section);
		    }
			int endInfo[3];
			pFile.read((char*)&endInfo, sizeof(endInfo));
			for (int i = 0; i < Sections.size(); i++)
			{
				if (Sections[i].type == MODEL_INFO_IDENT)
				{
					pFile.seekg(Sections[i].offset, pFile.beg);
					static_model_header minfo;
					pFile.read((char*)&minfo, sizeof(static_model_header));
					sModelInfo = minfo;
					
					Log(L"Verts: " + std::to_wstring(sModelInfo.vertices));
					Log(L"Faces: " + std::to_wstring(sModelInfo.faces));

					std::string str(sModelInfo.textName, strlen(sModelInfo.textName));
					std::wstring str2(str.length(), L' ');
					std::copy(str.begin(), str.end(), str2.begin());
					SetTextureText(str2);

				}

				if (Sections[i].type == VERTICES_IDENT)
				{
					pFile.seekg(Sections[i].offset, pFile.beg);

					std::vector<v_data> tmpVerts;
					std::vector<uv> tmpMaps;
					std::vector<vn> tmpNormals;
					std::vector<face> tmpFaces;
					std::vector<bone_link> tmpLinks;

		
					Miracle3DModel model;
					
					for (int x = 0; x < sModelInfo.vertices; x++)
					{
						v_data vdat;
						v vert;
						pFile.read((char*)&vert, sizeof(v));
						vdat.vert = vert;
						vn norm;
						pFile.read((char*)&norm, sizeof(vn));
						uv map;
						pFile.read((char*)&map, sizeof(uv));

						tmpVerts.push_back(vdat);
						tmpMaps.push_back(map);
						tmpNormals.push_back(norm);

					}
					for (int x = 0; x < sModelInfo.faces; x++)
					{
						face f;
						pFile.read((char*)&f, sizeof(face));
						tmpFaces.push_back(f);

					}
					model.Faces = tmpFaces;
					model.Verts = tmpVerts;
					model.Normals = tmpNormals;
					model.Maps = tmpMaps;
					tmpFaces.clear();
					tmpVerts.clear();
					tmpNormals.clear();
					tmpMaps.clear();
					
					Models.push_back(model);


				}

			}

		}


		ComboBox_SetCurSel(*hGroupBox, 0);
		std::wstring fileName = wsplitString(InputPath, true);
		SetWindowText(*hFileNameBox, InputPath.c_str());
		eApp::bIsIni = FALSE;
		eApp::bIsReady = TRUE;

	}
}

void M3DManager::ReadINI()
{
	if (InputPath.empty())
		return;

	if (!std::filesystem::exists(InputPath))
	{
			MessageBox(eApp::hWindow, L"Failed to open INI file!", L"Error", MB_ICONERROR);
			return;
	}

	Miracle3DDecompiledModel cfg;

	Log(L"Loading INI: " + InputPath);

	CIniReader reader(InputPath.c_str());

	cfg.type = reader.ReadInteger(L"Model", L"Type", 5);
	cfg.animationNumber = reader.ReadInteger(L"Model", L"Animations", 0);
	cfg.AnimatedData = reader.ReadString(L"Model", L"AnimationData", 0);
	cfg.TextureName = reader.ReadString(L"Model", L"TextureName", 0);
	cfg.Model = reader.ReadString(L"Model", L"Model", 0);
	Log(L"INI loaded!");
	Log(L"Type: " + std::to_wstring(cfg.type));
	Log(L"Texture: " + cfg.TextureName);
	if (cfg.type == M3D_MODEL_ANIMATED)
	Log(L"Anims: " + cfg.AnimatedData + L" - " + std::to_wstring(cfg.animationNumber));
	Log(L"Model: " + cfg.Model);



	SetTextureText(cfg.TextureName);
	SetWindowText(*hFileNameBox, InputPath.c_str());
	std::wstring compFolder = wsplitString(InputPath, false);
	SetWindowText(GetDlgItem(eApp::hWindow, M3D_COMPILE_FOLDER), compFolder.c_str());
	ModelConfig = cfg;
	eApp::bIsIni = TRUE;
	eApp::bIsReady = TRUE;
}

void M3DManager::Close()
{
	Skeleton.clear();
	Models.clear();
	Sections.clear();
	pFile.close();
	InputPath = L" ";
	OutputPath = L" ";
	SetWindowText(*hTextureNameBox, L"");
	SetWindowText(*hFileNameBox, L"");
	SendMessage(*hGroupBox, CB_RESETCONTENT, 0, 0);
	SetWindowText(*hLogBox, L"");
	ComboBox_SetCurSel(*hGroupBox, 0);
	WorkType = 0;
	
	eApp::bIsReady = false;
}

void M3DManager::Log(std::wstring msg)
{
	PushLogMessage(*hLogBox, msg);
}

void M3DManager::ExportToSMD(std::wstring folder, bool combine, bool flipUV)
{
	if (!std::filesystem::exists(folder))
		return;

	std::filesystem::current_path(folder);

	Log(L"Export folder: " + folder);

	// process smd



	if (ModelType == M3D_MODEL_STATIC)
	{
		std::wstring str = wsplitString(InputPath, true);
		str.replace(str.length() - 4, 4, L".smd");
		std::wstring name = str;

		std::ofstream oFile(name, std::ofstream::binary);


		wchar_t temp[256];
		wsprintf(temp, L"Saving  %s", name.c_str());
		Log(temp);

		oFile << "version 1\nnodes\n" << std::setprecision(4) << std::fixed;
		oFile << "0 \"root\" -1\n";
		oFile << "end\n";
		oFile << "skeleton\n";
		oFile << "time 0\n";
		oFile << "0 0.0 0.0 0.0 0.0 0.0 0.0\n";
		oFile << "end\n";
		oFile << "triangles\n";
		for (int i = 0; i < Models.size(); i++)
		{
			for (int a = 0; a < Models[i].Faces.size(); a++)
			{
				oFile << sModelInfo.textName<< std::endl;
				for (int k = 0; k < 3; k++)
				{
					v_data verts;
					vn     normal;
					uv     maps;
					verts = Models[i].Verts[Models[i].Faces[a].face[k]];
					normal = Models[i].Normals[Models[i].Faces[a].face[k]];

					maps = Models[i].Maps[Models[i].Faces[a].face[k]];

					oFile << 0 << " " << verts.vert.x << " " << verts.vert.y << " " << verts.vert.z << " "
						<< normal.norm[0] << " " << normal.norm[1] << " " << normal.norm[2] << " "
						<< maps.u << " " << 1.0 - maps.v << " " << 0;

					oFile << std::endl;
				}


			}

		}
		Log(L"Saved");
		oFile << "end\n";

	}

	else if (ModelType == M3D_MODEL_ANIMATED)
	{

		if (combine)
		{
			int firstModel = GetFirstAnimatedModel();

			if (firstModel < 0)
			{
				Log(L"No skinned objects in this model.");
				firstModel = 0;

			}

			std::string str = Models[firstModel].Name;
			std::wstring str2(str.length(), L' ');
			std::copy(str.begin(), str.end(), str2.begin());

			std::wstring name = str2;
			name += L".smd";

			Log(L"Saving: " + name);

			std::ofstream oFile(name, std::ofstream::binary);
			oFile << "version 1\nnodes\n" << std::setprecision(4) << std::fixed;



			for (int a = 0; a < Skeleton.size(); a++)
			{
				int parent = Skeleton[a].parentID;
				if (parent < 0) parent = -1;
				oFile << Skeleton[a].boneID << " \"" << Skeleton[a].boneName << "\" "
					<< parent << std::endl;
			}
			oFile << "end\n";
			oFile << "skeleton\n";
			oFile << "time 0\n";
			for (int a = 0; a < Skeleton.size(); a++)
			{
				vector3d pos = Skeleton[a].pos;
				quaternion3d qrot = { -Skeleton[a].rot.x, -Skeleton[a].rot.y,-Skeleton[a].rot.z,Skeleton[a].rot.w };
				vector3d tmprot = quat2vec(qrot);
				vector3d rot = { degToRad(tmprot.x), degToRad(tmprot.y) ,degToRad(tmprot.z) };

				oFile << Skeleton[a].boneID << " " << pos.x << " " << pos.y << " "
					<< pos.z << " " << rot.x << " " << rot.y << " " << rot.z << std::endl;
			}

			oFile << "end\n";
			oFile << "triangles\n";

			for (int i = 0; i < Models.size(); i++)
			{
				for (int a = 0; a < Models[i].Faces.size(); a++)
				{
					oFile << ModelInfo.textureName << std::endl;
					for (int k = 0; k < 3; k++)
					{
						v_data verts;
						vn     normal;
						uv     maps;
						verts = Models[i].Verts[Models[i].Faces[a].face[k]];
						normal = Models[i].Normals[Models[i].Faces[a].face[k]];

						maps = Models[i].Maps[Models[i].Faces[a].face[k]];

						oFile << 0 << " " << verts.vert.x << " " << verts.vert.y << " " << verts.vert.z << " "
							<< normal.norm[0] << " " << normal.norm[1] << " " << normal.norm[2] << " "
							<< maps.u << " ";

						if (flipUV)
							oFile << 1.0 - maps.v;
						else
							oFile << maps.v;

						if (verts.BoneLinks.size() == 0)
						{
							int boneID = GetBoneID(Models[i].Name);

							if (boneID < 0)
							{
								MessageBox(eApp::hWindow, L"Cannot combine the model as there's one or more objects without bone relation!", L"Error", MB_ICONERROR);
								return;
							}
							oFile << " " << 1 << " " << boneID << " " << 1.0f;
                         	oFile << std::endl;
						}

						else if (verts.BoneLinks.size() > 0)
						{
							oFile << " " << verts.BoneLinks.size();

							for (int x = 0; x < verts.BoneLinks.size(); x++)
							{
								oFile << " " << verts.BoneLinks[x].boneID << " " << verts.BoneLinks[x].weight;
							}
							oFile << std::endl;
						}
						else
							oFile << std::endl;
					}


				}

			}
			Log(L"Saved");
			oFile << "end\n";
			


		}
		else
		{
			for (int i = 0; i < Models.size(); i++)
			{
				std::string str = Models[i].Name;
				std::wstring str2(str.length(), L' ');
				std::copy(str.begin(), str.end(), str2.begin());

				std::wstring name = str2;
				name += L".smd";


				wchar_t temp[256];
				wsprintf(temp, L"Saving (%d/%d): %s", i + 1, Models.size(), name.c_str());
				Log(temp);

				std::ofstream oFile(name, std::ofstream::binary);
				if (Models[i].type == GROUP_ANIMATED)
				{
					oFile << "version 1\nnodes\n" << std::setprecision(4) << std::fixed;
					for (int a = 0; a < Skeleton.size(); a++)
					{
						int parent = Skeleton[a].parentID;
						if (parent < 0) parent = -1;
						oFile << Skeleton[a].boneID << " \"" << Skeleton[a].boneName << "\" "
							<< parent << std::endl;
					}
					oFile << "end\n";
					oFile << "skeleton\n";
					oFile << "time 0\n";
					for (int a = 0; a < Skeleton.size(); a++)
					{
						vector3d pos = Skeleton[a].pos;
						quaternion3d qrot = { -Skeleton[a].rot.x, -Skeleton[a].rot.y,-Skeleton[a].rot.z,Skeleton[a].rot.w };
						vector3d tmprot = quat2vec(qrot);
						vector3d rot = { degToRad(tmprot.x), degToRad(tmprot.y) ,degToRad(tmprot.z) };

						oFile << Skeleton[a].boneID << " " << pos.x << " " << pos.y << " "
							<< pos.z << " " << rot.x << " " << rot.y << " " << rot.z << std::endl;
					}
					oFile << "end\n";

					oFile << "triangles\n";

					for (int a = 0; a < Models[i].Faces.size(); a++)
					{
						oFile << ModelInfo.textureName << std::endl;
						for (int k = 0; k < 3; k++)
						{
							v_data verts;
							vn     normal;
							uv     maps;
							verts = Models[i].Verts[Models[i].Faces[a].face[k]];
							normal = Models[i].Normals[Models[i].Faces[a].face[k]];

							maps = Models[i].Maps[Models[i].Faces[a].face[k]];

							oFile << 0 << " " << verts.vert.x << " " << verts.vert.y << " " << verts.vert.z << " "
								<< normal.norm[0] << " " << normal.norm[1] << " " << normal.norm[2] << " "
								<< maps.u << " ";
							if (flipUV)
								oFile << 1.0 - maps.v;
							else
								oFile << maps.v;

							if (verts.BoneLinks.size() > 0)
							{
								oFile << " " << verts.BoneLinks.size();

								for (int x = 0; x < verts.BoneLinks.size(); x++)
								{
									oFile << " " << verts.BoneLinks[x].boneID << " " << verts.BoneLinks[x].weight;
								}
								oFile << std::endl;
							}
							else
								oFile << "0" << std::endl;
						}


					}
					Log(L"Saved");
					oFile << "end\n";
				}
				else if (Models[i].type == GROUP_STATIC || Models[i].type == GROUP_DUMMY)
				{
					oFile << "version 1\nnodes\n" << std::setprecision(4) << std::fixed;
					oFile << "0 \"default\" -1\n";
					oFile << "end\n";
					oFile << "skeleton\n";
					oFile << "time 0\n";
					oFile << "  0 0.000000 0.000000 0.000000 0.000000 -0.000000 0.000000\n";
					oFile << "end\n";

					oFile << "triangles\n";

					for (int a = 0; a < Models[i].Faces.size(); a++)
					{
						oFile << ModelInfo.textureName << std::endl;




						for (int k = 0; k < 3; k++)
						{
							v_data verts;
							vn     normal;
							uv     maps;
							verts = Models[i].Verts[Models[i].Faces[a].face[k]];
							normal = Models[i].Normals[Models[i].Faces[a].face[k]];

							maps = Models[i].Maps[Models[i].Faces[a].face[k]];

							oFile << 0 << " " << verts.vert.x << " " << verts.vert.y << " " << verts.vert.z << " "
								<< normal.norm[0] << " " << normal.norm[1] << " " << normal.norm[2] << " "
								<< maps.u << " " << 1.0 - maps.v << " " <<  0;

							oFile << std::endl;
						}


					}
					Log(L"Saved");
					oFile << "end\n";
				}
			}
		}
	}



}

void M3DManager::ExportToOBJ(std::wstring folder, bool combine, bool flipUV)
{

	if (!std::filesystem::exists(folder))
		return;

	std::filesystem::current_path(folder);

	Log(L"Export folder: " + folder);


	// process obj

	if (ModelType == M3D_MODEL_STATIC)
	{
		std::wstring str = wsplitString(InputPath, true);
		std::wstring groupName = wsplitString(InputPath, true);
		groupName = groupName.substr(0, groupName.length() - 4);
		str.replace(str.length() - 4, 4, L".obj");
		std::wstring name = str;

		std::ofstream oFile(name, std::ofstream::binary);


		wchar_t temp[256];
		wsprintf(temp, L"Saving  %s", name.c_str());
		Log(temp);

		oFile << "; obj created using Miracle3D Designer by ermaccer\n" << std::setprecision(4) << std::fixed;
		for (int i = 0; i < Models.size(); i++)
		{

			for (int a = 0; a < Models[i].Verts.size(); a++)
			{
				oFile << "v " << Models[i].Verts[a].vert.x << " " << Models[i].Verts[a].vert.y << " " << Models[i].Verts[a].vert.z << std::endl;
			}


			for (int a = 0; a < Models[i].Normals.size(); a++)
			{
				oFile << "vn " << Models[i].Normals[a].norm[0] << " " << Models[i].Normals[a].norm[1] << " " << Models[i].Normals[a].norm[2] << std::endl;
			}

			for (int a = 0; a < Models[i].Maps.size(); a++)
			{


				oFile << "vt " << Models[i].Maps[a].u << " ";
				if (flipUV)
					oFile << 1.0f - Models[i].Maps[a].v << std::endl;
				else
					oFile << Models[i].Maps[a].v << std::endl;
			}

			oFile << "g " << groupName.c_str() << std::endl;;
			char tmp[256];
			GetWindowTextA(*hTextureNameBox, tmp, 256);
			oFile << "usemtl  " << tmp << std::endl;

			for (int a = 0; a < Models[i].Faces.size(); a++)
			{
				oFile << "f " << Models[i].Faces[a].face[0] + 1 << "//" << Models[i].Faces[a].face[0] + 1 << "//" << Models[i].Faces[a].face[0] + 1
					<< " " << Models[i].Faces[a].face[1] + 1 << "//" << Models[i].Faces[a].face[1] + 1 << "//" << Models[i].Faces[a].face[1] + 1
					<< " " << Models[i].Faces[a].face[2] + 1 << "//" << Models[i].Faces[a].face[2] + 1 << "//" << Models[i].Faces[a].face[2] + 1 << std::endl;
			}
			Log(L"Saved");
		}

	}
	else if (ModelType == M3D_MODEL_ANIMATED)
	{

		if (combine)
		{
			MessageBox(eApp::hWindow, L"Can't combine models for OBJ export!", L"Error", MB_ICONERROR);
			return;
		}
		else
		{
			for (int i = 0; i < Models.size(); i++)
			{
				std::string str = Models[i].Name;
				std::wstring str2(str.length(), L' ');
				std::copy(str.begin(), str.end(), str2.begin());

				std::wstring name = str2;
				name += L".obj";

				wchar_t temp[256];
				wsprintf(temp, L"Saving  %s", name.c_str());
				Log(temp);

				std::ofstream oFile(name, std::ofstream::binary);

				oFile << "; obj created using Miracle3D Designer by ermaccer\n" << std::setprecision(4) << std::fixed;


				for (int a = 0; a < Models[i].Verts.size(); a++)
				{
					oFile << "v " << Models[i].Verts[a].vert.x << " " << Models[i].Verts[a].vert.y << " " << Models[i].Verts[a].vert.z << std::endl;
				}


				for (int a = 0; a < Models[i].Normals.size(); a++)
				{
					oFile << "vn " << Models[i].Normals[a].norm[0] << " " << Models[i].Normals[a].norm[1] << " " << Models[i].Normals[a].norm[2] << std::endl;
				}

				for (int a = 0; a < Models[i].Maps.size(); a++)
				{
					oFile << "vt " << Models[i].Maps[a].u << " ";
					if (flipUV)
						oFile << 1.0f - Models[i].Maps[a].v << std::endl;
					else
						oFile << Models[i].Maps[a].v << std::endl;
				}

				oFile << "g " << Models[i].Name << std::endl;;
				char tmp[256];
				GetWindowTextA(*hTextureNameBox, tmp, 256);
				oFile << "usemtl  " << tmp << std::endl;

				for (int a = 0; a < Models[i].Faces.size(); a++)
				{
					oFile << "f " << Models[i].Faces[a].face[0]  + 1<< "//" << Models[i].Faces[a].face[0] + 1 << "//" << Models[i].Faces[a].face[0] + 1
						<< " " << Models[i].Faces[a].face[1] + 1 << "//" << Models[i].Faces[a].face[1] + 1 << "//" << Models[i].Faces[a].face[1] + 1
						<< " " << Models[i].Faces[a].face[2] + 1 << "//" << Models[i].Faces[a].face[2] + 1 << "//" << Models[i].Faces[a].face[2] + 1 << std::endl;
				}
				Log(L"Saved");
			}

		}
	}
}

void M3DManager::OpenSMD(std::wstring file)
{
	if (file.empty())
		return;

	FILE* pFile = _wfopen(file.c_str(), L"rb");

	char szLine[1024];
	char* tempLine;
	Miracle3DModel model;
	Log(L"Opening: " + file);

	// first line must be version 1

	fgets(szLine, sizeof(szLine), pFile);


	if (!(strncmp(szLine, "version 1",9) == 0))
	{
		MessageBox(eApp::hWindow, L"SMD file is not valid!", L"Error", MB_ICONERROR);
		return;
	}

	Log(L"Opened SMD ok!");


	// check for nodes

	fgets(szLine, sizeof(szLine), pFile);

	if (!(strncmp(szLine, "nodes", 5) == 0))
	{
		std::string str = "ERROR: Expected nodes, got ";
		str += szLine;

		std::wstring str2(str.length(), L' ');
		std::copy(str.begin(), str.end(), str2.begin());

		Log(str2);

		return;
	}

	// read bone

	while (fgets(szLine, sizeof(szLine), pFile))
	{
		if (strncmp(szLine, "end", 3) == 0)
			break;

		if (szLine[0] == ';' || szLine[0] == '#' || szLine[0] == '\n')
			continue;
		tempLine = strtok(szLine, " ");

		int id;

		if (sscanf(szLine, "%d", &id) == 1)
		{
			char boneName[256] = {};
			bone_info bone;
			
			bone.boneID = id;

			int parent;
			tempLine = strtok(NULL, "\"");
			sprintf(bone.boneName, tempLine);
			tempLine = strtok(NULL, "\"");
			sscanf(tempLine, "%d", &parent);
			bone.parentID = parent;
			Skeleton.push_back(bone);
		}
	}

	Log(L"Loaded bones: " + std::to_wstring(Skeleton.size()));



	// check for skeleton

	fgets(szLine, sizeof(szLine), pFile);

	if (!(strncmp(szLine, "skeleton", 8) == 0))
	{
		std::string str = "ERROR: Expected skeleton, got ";
		str += szLine;

		std::wstring str2(str.length(), L' ');
		std::copy(str.begin(), str.end(), str2.begin());

		Log(str2);

		return;
	}

	// check for time 0 (only one frame!)

	fgets(szLine, sizeof(szLine), pFile);

	if (!(strncmp(szLine, "time 0", 6) == 0))
	{
		std::string str = "ERROR: Expected time 0, got ";
		str += szLine;

		std::wstring str2(str.length(), L' ');
		std::copy(str.begin(), str.end(), str2.begin());

		Log(str2);

		return;
	}

	// read bone pos

	while (fgets(szLine, sizeof(szLine), pFile))
	{
		if (strncmp(szLine, "end", 3) == 0)
			break;

		if (szLine[0] == ';' || szLine[0] == '#' || szLine[0] == '\n')
			continue;
		int id;

		if (sscanf(szLine, "%d", &id) == 1)
		{

			vector3d rot;
			vector3d pos;
			sscanf(szLine, "%d %f %f %f %f %f %f", &id, &pos.x, &pos.y, &pos.z, &rot.x, &rot.y, &rot.z);

			for (int i = 0; i < Skeleton.size(); i++)
			{
				if (Skeleton[i].boneID == id)
				{
					Skeleton[i].pos = pos;
					vector3d tmpRot = { radToDeg(pos.x), radToDeg(pos.y), radToDeg(pos.z) };
					quaternion3d tmprot = vec2quat(rot);
					quaternion3d qrot = { -tmprot.x, -tmprot.y,-tmprot.z,tmprot.w };
					Skeleton[i].rot = qrot;
				}
			}
		}
	}
/*
	for (int i = 0; i < Skeleton.size(); i++)
	{

		bone_info bone = Skeleton[i];
		std::string str(bone.boneName, strlen(bone.boneName));
		std::wstring str2(str.length(), L' ');
		std::copy(str.begin(), str.end(), str2.begin());
		//printf("Rot %f %f %f %f\n", bone.rot.x, bone.rot.y, bone.rot.z, bone.rot.w);
		wchar_t temp[256];
		wsprintf(temp, L"Bone (%s): ID - %d Parent - %d", str2.c_str(), bone.boneID, bone.parentID);
		Log(temp);
	}
	*/
	std::vector<v_data> tmpVerts;
	std::vector<uv> tmpMaps;
	std::vector<vn> tmpNormals;
	std::vector<face> tmpFaces;
	std::vector<bone_link> tmpLinks;


	// check for triangles

	fgets(szLine, sizeof(szLine), pFile);

	if (!(strncmp(szLine, "triangles", 9) == 0))
	{
		std::string str = "ERROR: Expected triangles, got ";
		str += szLine;

		std::wstring str2(str.length(), L' ');
		std::copy(str.begin(), str.end(), str2.begin());

		Log(str2);

		return;
	}

	// read triangles

	while (fgets(szLine, sizeof(szLine), pFile))
	{
		if (strncmp(szLine, "end", 3) == 0)
			break;

		if (szLine[0] == ';' || szLine[0] == '#' || szLine[0] == '\n')
			continue;
		tempLine = strtok(szLine, " ");

		int id;

		if (sscanf(szLine, "%d", &id) == 1)
		{
			unsigned short linksAmount;
			v_data v;
			vn norm;
			uv map;
			std::vector<bone_link> tmpLink;
			
			tempLine = strtok(NULL, " ");
			sscanf(tempLine, "%f", &v.vert.x);

			tempLine = strtok(NULL, " ");
			sscanf(tempLine, "%f", &v.vert.y);

			tempLine = strtok(NULL, " ");
			sscanf(tempLine, "%f", &v.vert.z);

			tempLine = strtok(NULL, " ");
			sscanf(tempLine, "%f", &norm.norm[0]);

			tempLine = strtok(NULL, " ");
			sscanf(tempLine, "%f", &norm.norm[1]);

			tempLine = strtok(NULL, " ");
			sscanf(tempLine, "%f", &norm.norm[2]);

			tempLine = strtok(NULL, " ");
			sscanf(tempLine, "%f", &map.u);

			tempLine = strtok(NULL, " ");
			sscanf(tempLine, "%f", &map.v);

			tempLine = strtok(NULL, " ");
			sscanf(tempLine, "%d", &linksAmount);



			for (int i = 0; i < linksAmount; i++)
			{
				bone_link link;
				tempLine = strtok(NULL, " ");
				sscanf(tempLine, "%d", &link.boneID);
				tempLine = strtok(NULL, " ");
				sscanf(tempLine, "%f", &link.weight);
				tmpLink.push_back(link);
			}
			v.BoneLinks = tmpLink;
			tmpVerts.push_back(v);
			tmpNormals.push_back(norm);
			tmpMaps.push_back(map);

		}
	}

	Log(L"Loaded vertices: " + std::to_wstring(tmpVerts.size()));

	int vertIndex = 0;

	std::vector<int> intFaces;
	std::vector<v_data> storedVertices;

	int begin = 0;

	for (int i = 0; i < tmpVerts.size(); i++)
	{
		intFaces.push_back(i);
	}

	Log(L"Generated faces: " + std::to_wstring(intFaces.size() / 3));


	int itr = 1;

	for (int i = 0; i < intFaces.size(); i+= 3)
	{
		face f = { intFaces[i + 0], intFaces[i + 1], intFaces[i + 2]};
		tmpFaces.push_back(f);
	}




	model.Name = Skeleton[0].boneName;
	model.type = 6;
	model.Maps = tmpMaps;
	model.Verts = tmpVerts;
	model.Normals = tmpNormals;
	model.Faces = tmpFaces;

	// logging
	int totalBoneLinks = 0;

	for (int i = 0; i < tmpVerts.size(); i++)
	{
		totalBoneLinks += tmpVerts[i].BoneLinks.size();
	}

	Log(L"Total bone links: " + std::to_wstring(totalBoneLinks));

	Models.push_back(model);
	fclose(pFile);
}

void M3DManager::Compile(std::wstring file, bool dontFlipUV)
{

	if (file.empty())
		return;

	wchar_t buffer[512];

	Models.clear();

	GetWindowText(GetDlgItem(eApp::hWindow, M3D_COMPILE_FOLDER), buffer, sizeof(buffer));

	std::wstring workingFolder(buffer, wcslen(buffer));
	if (workingFolder.length() <= 0)
	{
		MessageBox(eApp::hWindow, L"Working folder empty!", L"Error", MB_ICONERROR);
		return;
	}



	if (!std::filesystem::exists(workingFolder))
		return;

	std::ofstream oFile(file, std::ofstream::binary);
	Log(L"Model compilation started");



	Log(L"Working folder: " + workingFolder);

	std::filesystem::current_path(workingFolder);

	Log(L"Actual folder: " + std::filesystem::current_path().wstring());

	// model loaded
	OpenSMD(ModelConfig.Model);
	OptimizeForCompilation();
	if (ModelConfig.type == M3D_MODEL_ANIMATED)
	{
		// write sections

		m3d_header m3d;

		sprintf(m3d.header, "ecbnt,t");
		m3d.type = M3D_MODEL_ANIMATED;
		m3d.version = 1;

		oFile.write((char*)&m3d, sizeof(m3d_header));
		Log(L"Saved SAM Header");
		// write sections

		int baseOffset = sizeof(m3d_header) + (sizeof(m3d_section) *(m3d.type - 1)) + (sizeof(int) * 3);

		m3d_section section_modelInfo;
		section_modelInfo.offset = baseOffset;
		section_modelInfo.size = sizeof(model_info);
		section_modelInfo.type = MODEL_INFO_IDENT;

		oFile.write((char*)&section_modelInfo, sizeof(m3d_section));

		baseOffset += section_modelInfo.size;

		m3d_section section_boneInfo;
		section_boneInfo.offset = baseOffset;
		section_boneInfo.size = sizeof(bone_info) * Skeleton.size();
		section_boneInfo.type = BONE_INFO_IDENT;

		oFile.write((char*)&section_boneInfo, sizeof(m3d_section));

		baseOffset += section_boneInfo.size;

		m3d_section section_groupInfo;

		int groupInfoSize = sizeof(group_info) + Get3DDataSize(Models[0]);


		section_groupInfo.offset = baseOffset;
		section_groupInfo.size = groupInfoSize;
		section_groupInfo.type = GROUP_INFO_IDENT;

		oFile.write((char*)&section_groupInfo, sizeof(m3d_section));

		baseOffset += section_groupInfo.size;


		m3d_section section_animInfo;
		section_animInfo.offset = baseOffset;
		section_animInfo.size = std::filesystem::file_size(ModelConfig.AnimatedData);
		section_animInfo.type = ANIM_INFO_IDENT;

		oFile.write((char*)&section_animInfo, sizeof(m3d_section));

		baseOffset += section_animInfo.size;


		m3d_section section_endInfo;
		section_endInfo.offset = baseOffset;
		section_endInfo.size = sizeof(end_info);
		section_endInfo.type = END_INFO_IDENT;

		baseOffset += section_endInfo.size;

		oFile.write((char*)&section_endInfo, sizeof(m3d_section));

		int size, offset, pad = 0;
		size = sizeof(int);
		offset = baseOffset;

		oFile.write((char*)&size, sizeof(int));
		oFile.write((char*)&offset, sizeof(int));
		oFile.write((char*)&pad, sizeof(int));


		Log(L"Saved SAM Section Info");

		model_info m;
		m.actualBones = 0;
		m.boneAmount = Skeleton.size();
		m.skinnedObjects = 1;
		m.dummyObjects = 0;
		m.normalObjects = 0;
		m.animations = ModelConfig.animationNumber;

		std::wstring str = ModelConfig.TextureName;
		std::string str2(" ", str.length());
		std::copy(str.begin(), str.end(), str2.begin());
		sprintf(m.textureName, str2.c_str());

		oFile.write((char*)&m, sizeof(model_info));


		Log(L"Saved Model Header");



		for (int i = 0; i < Skeleton.size(); i++)
			oFile.write((char*)&Skeleton[i], sizeof(bone_info));


		Log(L"Saved Skeleton");



		group_info group;
		sprintf(group.name, Models[0].Name.c_str());
		group.faces = Models[0].Faces.size();
		group.verts = Models[0].Verts.size();
		group.type = 2;
		group.unk = 1;

		oFile.write((char*)&group, sizeof(group_info));


		Log(L"Saved Group Info");



		for (int i = 0; i < Models[0].Verts.size(); i++)
		{
			oFile.write((char*)&Models[0].Verts[i].vert, sizeof(v));
			oFile.write((char*)&Models[0].Normals[i], sizeof(vn));

			if (!dontFlipUV)
			{
				float temp = 1.0f - Models[0].Maps[i].v;
				Models[0].Maps[i].v = temp;
			}
				 

			oFile.write((char*)&Models[0].Maps[i], sizeof(uv));
			short boneLinks = Models[0].Verts[i].BoneLinks.size();

			oFile.write((char*)&boneLinks, sizeof(short));
			if (boneLinks > 0)
				for (int k = 0; k < boneLinks; k++)
					oFile.write((char*)&Models[0].Verts[i].BoneLinks[k], sizeof(bone_link));
		}






		for (int i = 0; i < Models[0].Faces.size(); i++)
			oFile.write((char*)&Models[0].Faces[i], sizeof(face));
		Log(L"Saved Group Geometry Data");
		std::ifstream pAnims(ModelConfig.AnimatedData, std::ifstream::binary);

		int animSize = (int)std::filesystem::file_size(ModelConfig.AnimatedData);

		std::unique_ptr<char[]> animBuff = std::make_unique<char[]>(animSize);
		pAnims.read(animBuff.get(), animSize);
		oFile.write(animBuff.get(), animSize);
		Log(L"Saved Animation Data");

		end_info e;
		e.header = '\0MAS';
		oFile.write((char*)&e, sizeof(end_info));

		int end = 1;
		oFile.write((char*)&end, sizeof(int));
		Log(L"Saved End Data");

	}
	else if (ModelConfig.type = M3D_MODEL_STATIC)
	{

		m3d_header m3d;

		sprintf(m3d.header, "ecbnt,t");
		m3d.type = M3D_MODEL_STATIC;
		m3d.version = 1;

		oFile.write((char*)&m3d, sizeof(m3d_header));
		Log(L"Saved GSM Header");

		int baseOffset = sizeof(m3d_header) + (sizeof(m3d_section) *(m3d.type - 1)) + (sizeof(int) * 3);

		m3d_section section_modelInfo;
		section_modelInfo.offset = baseOffset;
		section_modelInfo.size = sizeof(static_model_header);
		section_modelInfo.type = MODEL_INFO_IDENT;

		oFile.write((char*)&section_modelInfo, sizeof(m3d_section));

		baseOffset += section_modelInfo.size;

		m3d_section section_vertInfo;
		section_vertInfo.offset = baseOffset;
		section_vertInfo.size = GetVertDataSize(Models[0]);
		section_vertInfo.type = VERTICES_IDENT;

		oFile.write((char*)&section_vertInfo, sizeof(m3d_section));

		baseOffset += section_vertInfo.size;

		m3d_section section_faceInfo;
		section_faceInfo.offset = baseOffset;
		section_faceInfo.size = GetFaceDataSize(Models[0]);
		section_faceInfo.type = FACES_IDENT;

		oFile.write((char*)&section_faceInfo, sizeof(m3d_section));

		baseOffset += section_faceInfo.size;

		m3d_section section_endInfo;
		section_endInfo.offset = baseOffset;
		section_endInfo.size = sizeof(end_info);
		section_endInfo.type = END_INFO_IDENT;

		baseOffset += section_endInfo.size;

		oFile.write((char*)&section_endInfo, sizeof(m3d_section));

		int size, offset, pad = 0;
		size = sizeof(int);
		offset = baseOffset;

		oFile.write((char*)&size, sizeof(int));
		oFile.write((char*)&offset, sizeof(int));
		oFile.write((char*)&pad, sizeof(int));

		static_model_header mi;
		mi.unk = 1;
		mi.type = 1;
		mi.header = 'MSSG';
		mi.faces = Models[0].Faces.size();
		mi.vertices = Models[0].Verts.size();
		Log(L"Texture: " + ModelConfig.TextureName);
		std::wstring str = ModelConfig.TextureName;
		std::string str2(" ", str.length());
		std::copy(str.begin(), str.end(), str2.begin());
		sprintf(mi.textName, str2.c_str());


		oFile.write((char*)&mi, sizeof(static_model_header));


		Log(L"Saved Model Header");


		for (int i = 0; i < Models[0].Verts.size(); i++)
		{
			oFile.write((char*)&Models[0].Verts[i].vert, sizeof(v));
			oFile.write((char*)&Models[0].Normals[i], sizeof(vn));
			if (!dontFlipUV)
			{
				float temp = 1.0f - Models[0].Maps[i].v;
				Models[0].Maps[i].v = temp;
			}


			oFile.write((char*)&Models[0].Maps[i], sizeof(uv));
		}


		for (int i = 0; i < Models[0].Faces.size(); i++)
			oFile.write((char*)&Models[0].Faces[i], sizeof(face));

		Log(L"Saved Geometry");
		end_info e;
		e.header = '\0MSG';
		oFile.write((char*)&e, sizeof(end_info));

		int end = 1;
		oFile.write((char*)&end, sizeof(int));
		Log(L"Saved End Data");

	}
	
}

void M3DManager::OptimizeForCompilation()
{
	if (!IsDlgButtonChecked(eApp::hWindow, M3D_GEO_OPTIMIZE))
		return;

	// max optimize?

	Log(L"Preparing to optimize");

	if (Models.size() > 1)
	{
		MessageBox(eApp::hWindow, L"Model is already optimized", L"Error", MB_ICONERROR);
		return;
	}

	std::vector<v_data> copyVerts = Models[0].Verts;

	// generate new faces

	std::vector<face> faces;
	std::vector<int> faceIDs;
	std::vector<v_data> newVerts;
	std::vector<uv> newMaps;
	std::vector<vn> newNorm;

	for (int i = 0; i < Models[0].Faces.size(); i++)
	{
		face f = Models[0].Faces[i];

		int first =	FindFirstVertexNumber(copyVerts, copyVerts[f.face[0]]);
		int second = FindFirstVertexNumber(copyVerts, copyVerts[f.face[1]]);
		int third =	FindFirstVertexNumber(copyVerts, copyVerts[f.face[2]]);

		face newFace = { first,second,third };

		faceIDs.push_back(first);
		faceIDs.push_back(second);
		faceIDs.push_back(third);
		faces.push_back(newFace);

	}

	std::sort(faceIDs.begin(), faceIDs.end());
	faceIDs.erase(std::unique(faceIDs.begin(), faceIDs.end()), faceIDs.end());

	for (int i = 0; i < faceIDs.size(); i++)
	{
		newVerts.push_back(copyVerts[faceIDs[i]]);
		newMaps.push_back(Models[0].Maps[faceIDs[i]]);
		newNorm.push_back(Models[0].Normals[faceIDs[i]]);
	}
	

	Log(L"Verts before optimization: " + std::to_wstring(Models[0].Verts.size()));
	Log(L"Verts after optimization: " + std::to_wstring(newVerts.size()));

	// generate final faces


	std::vector<face> readyFaces;

	for (int i = 0; i < faces.size(); i++)
	{
		face f = faces[i];

		int first  = FindFirstVertexNumber(newVerts, copyVerts[f.face[0]]);
		int second  = FindFirstVertexNumber(newVerts, copyVerts[f.face[1]]);
		int third = FindFirstVertexNumber(newVerts, copyVerts[f.face[2]]);

		face newFace = { first, second, third };
		readyFaces.push_back(newFace);
	}
	
/*
	for (int i = 0; i < newVerts.size(); i++)
		printf("v %f %f %f\n", newVerts[i].vert.x, newVerts[i].vert.y, newVerts[i].vert.z);

	for (int i = 0; i < newMaps.size(); i++)
		printf("vt %f %f\n", newMaps[i].u, 1.0 - newMaps[i].v);

	for (int i = 0; i < newNorm.size(); i++)
		printf("vn %f %f %f\n", newNorm[i].norm[0], newNorm[i].norm[1], newNorm[i].norm[2]);

	for (int i = 0; i < readyFaces.size(); i++)
		printf("f %d/%d/%d %d/%d/%d %d/%d/%d\n", readyFaces[i].face[0] + 1, readyFaces[i].face[0] + 1, readyFaces[i].face[0] + 1, 
			readyFaces[i].face[1] + 1, readyFaces[i].face[1] + 1, readyFaces[i].face[1] + 1, 
			readyFaces[i].face[2] + 1, readyFaces[i].face[2] + 1, readyFaces[i].face[2] + 1);
			*/

	Models[0].Verts = newVerts;
	Models[0].Faces = readyFaces;
	Models[0].Maps = newMaps;
	Models[0].Normals = newNorm;
}

void M3DManager::Decompile(std::wstring folder)
{
	if (!std::filesystem::exists(folder))
		return;

	std::filesystem::current_path(folder);

	Log(L"Decompilation folder: " + folder);

	std::string mName;

	if (ModelType == M3D_MODEL_ANIMATED)
	{
		int firstModel = GetFirstAnimatedModel();

		bool flipUV = true;

		if (firstModel < 0)
		{
			Log(L"No skinned objects in this model.");
			firstModel = 0;

		}

		std::string str = Models[firstModel].Name;
		std::wstring str2(str.length(), L' ');
		std::copy(str.begin(), str.end(), str2.begin());
		mName = str;
		std::wstring name = str2;
		name += L".smd";

		Log(L"Saving: " + name);

		std::ofstream oFile(name, std::ofstream::binary);
		oFile << "version 1\nnodes\n" << std::setprecision(4) << std::fixed;



		for (int a = 0; a < Skeleton.size(); a++)
		{
			int parent = Skeleton[a].parentID;
			if (parent < 0) parent = -1;
			oFile << Skeleton[a].boneID << " \"" << Skeleton[a].boneName << "\" "
				<< parent << std::endl;
		}
		oFile << "end\n";
		oFile << "skeleton\n";
		oFile << "time 0\n";
		for (int a = 0; a < Skeleton.size(); a++)
		{
			vector3d pos = Skeleton[a].pos;
			quaternion3d qrot = { -Skeleton[a].rot.x, -Skeleton[a].rot.y,-Skeleton[a].rot.z,Skeleton[a].rot.w };
			vector3d tmprot = quat2vec(qrot);
			vector3d rot = { degToRad(tmprot.x), degToRad(tmprot.y) ,degToRad(tmprot.z) };

			oFile << Skeleton[a].boneID << " " << pos.x << " " << pos.y << " "
				<< pos.z << " " << rot.x << " " << rot.y << " " << rot.z << std::endl;
		}

		oFile << "end\n";
		oFile << "triangles\n";

		for (int i = 0; i < Models.size(); i++)
		{
			for (int a = 0; a < Models[i].Faces.size(); a++)
			{
				oFile << ModelInfo.textureName << std::endl;
				for (int k = 0; k < 3; k++)
				{
					v_data verts;
					vn     normal;
					uv     maps;
					verts = Models[i].Verts[Models[i].Faces[a].face[k]];
					normal = Models[i].Normals[Models[i].Faces[a].face[k]];

					maps = Models[i].Maps[Models[i].Faces[a].face[k]];

					oFile << 0 << " " << verts.vert.x << " " << verts.vert.y << " " << verts.vert.z << " "
						<< normal.norm[0] << " " << normal.norm[1] << " " << normal.norm[2] << " "
						<< maps.u << " ";

					if (flipUV)
						oFile << 1.0 - maps.v;
					else
						oFile << maps.v;

					if (verts.BoneLinks.size() == 0)
					{
						int boneID = GetBoneID(Models[i].Name);

						if (boneID < 0)
						{
							MessageBox(eApp::hWindow, L"Cannot combine the model as there's one or more objects without bone relation!", L"Error", MB_ICONERROR);
							return;
						}
						oFile << " " << 1 << " " << boneID << " " << 1.0f;
						oFile << std::endl;
					}

					else if (verts.BoneLinks.size() > 0)
					{
						oFile << " " << verts.BoneLinks.size();

						for (int x = 0; x < verts.BoneLinks.size(); x++)
						{
							oFile << " " << verts.BoneLinks[x].boneID << " " << verts.BoneLinks[x].weight;
						}
						oFile << std::endl;
					}
					else
						oFile << std::endl;
				}


			}

		}
		Log(L"Saved");
		oFile << "end\n";

	}
	else if (ModelType == M3D_MODEL_STATIC)
	{
	std::wstring str = wsplitString(InputPath, true);
	std::string tmp(" ", str.length());
	std::copy(str.begin(), str.end(), tmp.begin());
	tmp = tmp.substr(0, tmp.length() - 4);
	mName = tmp;
	str.replace(str.length() - 4, 4, L".smd");
	std::wstring name = str;


	std::ofstream oFile(name, std::ofstream::binary);


	wchar_t temp[256];
	wsprintf(temp, L"Saving  %s", name.c_str());
	Log(temp);

	oFile << "version 1\nnodes\n" << std::setprecision(4) << std::fixed;
	oFile << "0 \"root\" -1\n";
	oFile << "end\n";
	oFile << "skeleton\n";
	oFile << "time 0\n";
	oFile << "0 0.0 0.0 0.0 0.0 0.0 0.0\n";
	oFile << "end\n";
	oFile << "triangles\n";
	for (int i = 0; i < Models.size(); i++)
	{
		for (int a = 0; a < Models[i].Faces.size(); a++)
		{
			oFile << sModelInfo.textName << std::endl;
			for (int k = 0; k < 3; k++)
			{
				v_data verts;
				vn     normal;
				uv     maps;
				verts = Models[i].Verts[Models[i].Faces[a].face[k]];
				normal = Models[i].Normals[Models[i].Faces[a].face[k]];

				maps = Models[i].Maps[Models[i].Faces[a].face[k]];

				oFile << 0 << " " << verts.vert.x << " " << verts.vert.y << " " << verts.vert.z << " "
					<< normal.norm[0] << " " << normal.norm[1] << " " << normal.norm[2] << " "
					<< maps.u << " " << 1.0 - maps.v << " " << 0;

				oFile << std::endl;
			}


		}

	}
	Log(L"Saved");
	oFile << "end\n";

	}

	







	Log(L"Saved SMD!");
	// dump animation info


	std::wstring animInfo = L"animationData.bin";


	Log(L"Generating INI...");

	std::ofstream ini(L"model.ini", std::ofstream::binary);

	ini << "[Model]\n";
	ini << "Type=" << ModelType << std::endl;
	if (ModelType == M3D_MODEL_ANIMATED)
	{
		ini << "Animations=" << ModelInfo.animations << std::endl;
		ini << "TextureName=" << ModelInfo.textureName << std::endl;
		ini << "AnimationData=" << "animationData.bin" << std::endl;
	}
	else if (ModelType == M3D_MODEL_STATIC)
	{
		ini << "TextureName=" << sModelInfo.textName << std::endl;
	}




	ini << "Model=" << mName << ".smd" << std::endl;


	// dump sections
	Log(L"Enumerating sections...");
	for (int i = 0; i < Sections.size(); i++)
	{
		if (Sections[i].type == ANIM_INFO_IDENT)
		{
			Log(L"Saving animation data:");
			pFile.seekg(Sections[i].offset, pFile.beg);
			if (ModelType == M3D_MODEL_ANIMATED)
			{

				for (int k = 0; k < ModelInfo.animations; k++)
				{
					ini << "[Anim" << k << "]\n";
					anim_header anim;
					pFile.read((char*)&anim, sizeof(anim_header));

					std::string str = anim.name;

					std::wstring wstr(str.begin(), str.end());
					wstr += L".smd";
					ini << "ID=" << anim.animID << std::endl;
					ini << "Framerate=" << anim.framerate << std::endl;
					ini << "UnkAmount=" << anim.unkAmount << std::endl;
					ini << "Name=" << anim.name << std::endl;
					ini << "File=" << str  << ".smd" << std::endl;



					std::ofstream oFile(wstr, std::ofstream::binary);

					for (int a = 0; a < anim.unkAmount; a++)
					{
						int unk[2];
						pFile.read((char*)&unk, sizeof(unk));
					}

					oFile << "version 1\nnodes\n" << std::setprecision(4) << std::fixed;


					for (int a = 0; a < Skeleton.size(); a++)
					{
						int parent = Skeleton[a].parentID;
						if (parent < 0) parent = -1;
						oFile << Skeleton[a].boneID << " \"" << Skeleton[a].boneName << "\" "
							<< parent << std::endl;
					}

					oFile << "end\n";
					oFile << "skeleton\n";
					for (int a = 0; a < anim.frames; a++)
					{

						oFile << "time " << a << std::endl;

						for (int x = 0; x < anim.bones; x++)
						{
							anim_frame frame;
							pFile.read((char*)&frame, sizeof(anim_frame));

							vector3d pos = frame.pos;
							quaternion3d qrot = { -frame.rot.x, -frame.rot.y,-frame.rot.z,frame.rot.w };
							vector3d tmprot = quat2vec(qrot);
							vector3d rot = { degToRad(tmprot.x), degToRad(tmprot.y) ,degToRad(tmprot.z) };

							oFile << frame.boneID << " " << pos.x << " " << pos.y << " "
								<< pos.z << " " << rot.x << " " << rot.y << " " << rot.z << std::endl;

						}

	
					}
					oFile << "end\n"; 
					Log(L"Saved anim: " + wstr);
				}
			}

		

			pFile.seekg(Sections[i].offset, pFile.beg);
			int size = Sections[i].size;
			std::unique_ptr<char[]> dataBuff = std::make_unique<char[]>(size);

			
			pFile.read(dataBuff.get(), size);

			std::ofstream animDump(animInfo, std::ofstream::binary);
			animDump.write(dataBuff.get(), size);
			Log(L"Saved animation data!");
		}
	}


	Log(L"INI generated!");

	
}

void M3DManager::ReadSMDAnim(std::wstring file)
{
	if (file.empty())
		return;

	FILE* pFile = _wfopen(file.c_str(), L"rb");

	char szLine[1024];
	char* tempLine;
	Miracle3DModel model;
	Miracle3DAnimation anim;
	Log(L"Opening: " + file);

	// first line must be version 1

	fgets(szLine, sizeof(szLine), pFile);


	if (!(strncmp(szLine, "version 1", 9) == 0))
	{
		MessageBox(eApp::hWindow, L"SMD file is not valid!", L"Error", MB_ICONERROR);
		return;
	}

	Log(L"Opened SMD ok!");
}

void M3DManager::SetTextureText(std::wstring text)
{
	SetWindowText(*hTextureNameBox, text.c_str());
}

int M3DManager::GetFirstAnimatedModel()
{
	int iFind = -1;

	for (int i = 0; i < Models.size(); i++)
	{
		if (Models[i].type == GROUP_ANIMATED)
		{
			iFind = i;
			break;
		}
	}
	return iFind;
}

int M3DManager::GetBoneID(std::string name)
{
	int iFind = -1;

	for (int i = 0; i < Skeleton.size(); i++)
	{
		std::string str = Skeleton[i].boneName;
		if (str == name)
		{
			iFind = Skeleton[i].boneID;
			break;
		}
	}
	return iFind;
}

int M3DManager::Get3DDataSize(Miracle3DModel model)
{
	int verticesSize = model.Verts.size() * sizeof(v);
	int boneLinksSize = 0;
	for (int i = 0; i < model.Verts.size(); i++)
	{
		boneLinksSize += model.Verts[i].BoneLinks.size() * sizeof(bone_link);
		boneLinksSize += sizeof(short);
	}
	int mapsSize = model.Maps.size() * sizeof(uv);
	int normSize = model.Normals.size() * sizeof(vn);
	int faceSize = model.Faces.size() * sizeof(face);

	int totalSize = verticesSize + boneLinksSize + mapsSize + normSize + faceSize;

	return totalSize;
}

int M3DManager::GetVertDataSize(Miracle3DModel model)
{
	int verticesSize = model.Verts.size() * sizeof(v);
	int mapsSize = model.Maps.size() * sizeof(uv);
	int normSize = model.Normals.size() * sizeof(vn);

	int totalSize = verticesSize + mapsSize + normSize;

	return totalSize;
}

int M3DManager::GetFaceDataSize(Miracle3DModel model)
{
	return  model.Faces.size() * sizeof(face);
}

bool IsValidM3DFile(int id)
{
	if (id == M3D_MODEL_STATIC || id == M3D_MODEL_ANIMATED)
		return true;
	else 
		return false;
}

char * GetModelInfo(int id)
{
	switch (id)
	{
	case GROUP_STATIC:
		return "STATIC"; break;
	case GROUP_ANIMATED:
		return "ANIMATED"; break;
	case GROUP_DUMMY:
		return "DUMMY"; break;
	}
}

wchar_t * GetModelType(int id)
{
	if (id == M3D_MODEL_ANIMATED)
		return L"Animated";
	else
		return L"Static";
}


std::wstring SetPathFromButton(wchar_t* filter, wchar_t* ext, HWND hWnd)
{
	wchar_t szBuffer[MAX_PATH] = {};
	OPENFILENAME ofn = {};

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = filter;
	ofn.lpstrFile = szBuffer;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = ext;
	std::wstring out;
	if (GetOpenFileName(&ofn))
		out = szBuffer;

	return out;
}


std::wstring  SetSavePathFromButton(wchar_t* filter, wchar_t* ext, HWND hWnd)
{
	wchar_t szBuffer[MAX_PATH] = {};
	OPENFILENAME ofn = {};

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = filter;
	ofn.lpstrFile = szBuffer;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
	ofn.lpstrDefExt = ext;
	std::wstring out;
	if (GetSaveFileName(&ofn))
		out = szBuffer;

	return out;
}

std::wstring   SetFolderFromButton(HWND hWnd)
{
	wchar_t szBuffer[MAX_PATH];

	BROWSEINFO bi = {};
	bi.lpszTitle = L"Select Folder";
	bi.hwndOwner = hWnd;
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

	LPITEMIDLIST idl = SHBrowseForFolder(&bi);

	std::wstring out;

	if (idl)
	{
		SHGetPathFromIDList(idl, szBuffer);
		out = szBuffer;

	}

	return out;
}

void PushLogMessage(HWND hWnd, std::wstring msg)
{
	msg += L"\r\n";
	int len = SendMessage(hWnd, WM_GETTEXTLENGTH, 0, 0);
	SendMessage(hWnd, EM_SETSEL, (WPARAM)len, (LPARAM)len);
	SendMessage(hWnd, EM_REPLACESEL, FALSE, (LPARAM)msg.c_str());

}

v_data FindFirstVertex(std::vector<v_data> data, v_data v)
{
	v_data result;
	for (int i = 0; i < data.size(); i++)
	{
		if (areVertsEqual(data[i], v))
		{
			result = data[i];
			break;
		}
	}
	return result;
}

int FindFirstVertexNumber(std::vector<v_data> data, v_data v)
{
	int result = 0;
	for (int i = 0; i < data.size(); i++)
	{
		if (areVertsEqual(v, data[i]))
		{
			result = i;
			break;
		}
	}
	return result;
}

