#include "modeldata.h"

#include <assimp/cimport.h>       // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

#include <QtOpenGL>
#include <QFileInfo>



//////////////////////////////////////
//Public 
//////////////////////////////////////
ModelData::ModelData(B9Layout* main)
{
	pMain = main;
	filepath = "";
	loadedcount=0;

	maxbound = QVector3D(-999999.0,-999999.0,-999999.0);
	minbound = QVector3D(999999.0,999999.0,999999.0);

}
ModelData::~ModelData()
{
    unsigned int i;
	for(i=0; i < instList.size();i++)
	{
		delete instList[i];
	}
	glDeleteLists(displaylistindx,1);
	triList.clear();
}

QString ModelData::GetFilePath()
{	
	return filepath;
}
QString ModelData::GetFileName()
{
	return filename;
}

//data loading
bool ModelData::LoadIn(QString filepath)
{	
    unsigned int m;
    unsigned int t;
    unsigned int i;

	
	this->filepath = filepath;
	
	if(filepath.isEmpty())
		return false;
	
	//extract filename from path!
	filename = QFileInfo(filepath).baseName();

	//AI_CONFIG_PP_FD_REMOVE = aiPrimitiveType_POINTS | aiPrimitiveType_LINES;
    pScene = aiImportFile(filepath.toAscii(), aiProcess_Triangulate);// | aiProcess_JoinIdenticalVertices); //trian
	
	if(pScene == NULL)
	{
		//display Assimp Error
		QMessageBox msgBox;
		msgBox.setText("Assimp Error:  " + QString().fromAscii(aiGetErrorString()));
		msgBox.exec();
		return false;
	}

	qDebug() << pScene->mMeshes[0]->mNumFaces;
	
	qDebug() << pScene->mNumMeshes;
	for (m = 0; m < pScene->mNumMeshes; m++) 
	{
		const aiMesh* mesh = pScene->mMeshes[m];
		
	    for (t = 0; t < mesh->mNumFaces; t++)
		{
			const struct aiFace* face = &mesh->mFaces[t];
			
			if(face->mNumIndices == 3)
			{
				Triangle3D newtri;
				for(i = 0; i < face->mNumIndices; i++) 
				{
					int index = face->mIndices[i];
				
					newtri.normal.setX(mesh->mNormals[index].x);
					newtri.normal.setY(mesh->mNormals[index].y);
					newtri.normal.setZ(mesh->mNormals[index].z);
			
					newtri.vertex[i].setX(mesh->mVertices[index].x);
					newtri.vertex[i].setY(mesh->mVertices[index].y);
					newtri.vertex[i].setZ(mesh->mVertices[index].z);
				}
				newtri.UpdateBounds();
				triList.push_back(newtri);
			}
		}
	}

	aiReleaseImport(pScene);

	qDebug() << "Loaded triangles: " << triList.size();
	//now center it!
	CenterModel();

	//generate a displaylist
	FormDisplayList();

	return true;
}

//instance
ModelInstance* ModelData::AddInstance()
{
	loadedcount++;
	ModelInstance* pNewInst = new ModelInstance(this);
	instList.push_back(pNewInst);
	pNewInst->SetTag(filename + " " + QString().number(loadedcount));
	return pNewInst;
}


//////////////////////////////////////
//Private
//////////////////////////////////////
void ModelData::CenterModel()
{
	//figure out what to current center of the models counds is..
    unsigned int f;
    unsigned int v;
	QVector3D center;


	for(f=0;f<triList.size();f++)
	{
		for(v=0;v<3;v++)
		{
			//update the models bounds.
			//max
			if(triList[f].maxBound.x() > maxbound.x())
			{
				maxbound.setX(triList[f].vertex[v].x());
			}
			if(triList[f].maxBound.y() > maxbound.y())
			{
				maxbound.setY(triList[f].vertex[v].y());
			}
			if(triList[f].maxBound.z() > maxbound.z())
			{
				maxbound.setZ(triList[f].vertex[v].z());
			}
			
			//mins
			if(triList[f].minBound.x() < minbound.x())
			{
				minbound.setX(triList[f].vertex[v].x());
			}
			if(triList[f].minBound.y() < minbound.y())
			{
				minbound.setY(triList[f].vertex[v].y());
			}
			if(triList[f].minBound.z() < minbound.z())
			{
				minbound.setZ(triList[f].vertex[v].z());
			}
		}
	}

	center = (maxbound + minbound)*0.5;

	for(f=0;f<triList.size();f++)
	{
		for(v=0;v<3;v++)
		{
			triList[f].vertex[v] -= center;
		}
		triList[f].UpdateBounds(); // since we are moving every triangle, we need to update their bounds too.
	}
	maxbound -= center;
	minbound -= center;
}

//rendering
void ModelData::FormDisplayList()
{
    unsigned int t;
	displaylistindx = glGenLists(1);//get an available display index
	if (displaylistindx != 0) 
	{
		glNewList(displaylistindx,GL_COMPILE);
		glBegin(GL_TRIANGLES);// Drawing Using Triangles
		for(t = 0; t < triList.size(); t++)//for each triangle
		{
			glNormal3f(triList[t].normal.x(),triList[t].normal.y(),triList[t].normal.z());//normals
			
				glVertex3f( triList[t].vertex[0].x(), triList[t].vertex[0].y(), triList[t].vertex[0].z());     
				glVertex3f( triList[t].vertex[1].x(), triList[t].vertex[1].y(), triList[t].vertex[1].z());     
				glVertex3f( triList[t].vertex[2].x(), triList[t].vertex[2].y(), triList[t].vertex[2].z());     
		}
		glEnd();
		glEndList();
	}
}
