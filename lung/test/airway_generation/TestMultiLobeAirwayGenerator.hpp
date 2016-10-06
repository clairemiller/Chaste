/*

Copyright (c) 2005-2016, University of Oxford.
All rights reserved.

University of Oxford means the Chancellor, Masters and Scholars of the
University of Oxford, having an administrative office at Wellington
Square, Oxford OX1 2JD, UK.

This file is part of Chaste.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
 * Neither the name of the University of Oxford nor the names of its
   contributors may be used to endorse or promote products derived from this
   software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef TESTMULTILOBEAIRWAYGENERATOR_HPP_
#define TESTMULTILOBEAIRWAYGENERATOR_HPP_

#include <cxxtest/TestSuite.h>
#include "MultiLobeAirwayGenerator.hpp"
#include "OutputFileHandler.hpp"
#include "TetrahedralMesh.hpp"

//This test is always run sequentially (never in parallel)
#include "FakePetscSetup.hpp"

#ifdef CHASTE_VTK

#define _BACKWARD_BACKWARD_WARNING_H 1 //Cut out the strstream deprecated warning for now (gcc4.3)
#include "vtkVersion.h"
#include "vtkSmartPointer.h"
#include "vtkPolyData.h"
#include "vtkPointData.h"
#include "vtkSphereSource.h"
#include "vtkCubeSource.h"
#include "vtkXMLPolyDataWriter.h"
#include "vtkSTLReader.h"
#include "vtkTriangleFilter.h"


#endif //CHASTE_VTK

class TestMultiLobeAirwayGenerator : public CxxTest::TestSuite
{
public:
    void TestAddLobes() throw(Exception)
    {
#if defined(CHASTE_VTK) && ( (VTK_MAJOR_VERSION >= 5 && VTK_MINOR_VERSION >= 6) || VTK_MAJOR_VERSION >= 6)

        EXIT_IF_PARALLEL;

        TetrahedralMesh<1,3> airways_mesh;

        MultiLobeAirwayGenerator generator(airways_mesh);

        generator.AddLobe(CreateSphere(-2.0, 0.0, 0.0), LEFT);
        generator.AddLobe(CreateSphere(0.0, 3.0, 0.0), LEFT);
        generator.AddLobe(CreateSphere(2.0, 0.0, 0.0), RIGHT);

        //Check the correct number of lobes were added
        TS_ASSERT_EQUALS(generator.GetNumLobes(LEFT), 2u);
        TS_ASSERT_EQUALS(generator.GetNumLobes(RIGHT), 1u);

        generator.AddLobe("lung/test/data/rll.stl", RIGHT);
        TS_ASSERT_EQUALS(generator.GetNumLobes(RIGHT), 2u);
#endif
    }

    void TestAssignGrowthApicesAndDistributePoints() throw(Exception)
    {
#if defined(CHASTE_VTK) && ( (VTK_MAJOR_VERSION >= 5 && VTK_MINOR_VERSION >= 6) || VTK_MAJOR_VERSION >= 6)

        EXIT_IF_PARALLEL;

        //Major airways mesh has 4 end points at (+/-2,0,0) and (0,+/-2,0), one is unused
        TetrahedralMesh<1,3> airways_mesh;
        TrianglesMeshReader<1,3> airways_mesh_reader("lung/test/airway_generation/data/test_major_airways_mesh");
        airways_mesh.ConstructFromMeshReader(airways_mesh_reader);

        MultiLobeAirwayGenerator generator(airways_mesh);

        generator.SetNumberOfPointsPerLung(0);
        TS_ASSERT_THROWS_CONTAINS(generator.DistributePoints(), "Must call SetNumberOfPointsPerLung or SetPointVolume before distributing points.");

        generator.SetNumberOfPointsPerLung(50);
        generator.SetMinimumBranchLength(1.2);
        generator.SetPointLimit(5);
        generator.SetAngleLimit(90.0);
        generator.SetBranchingFraction(0.5);
        generator.SetDiameterRatio(1.6);

        generator.AddLobe(CreateSphere(-2.0, 0.0, 0.0), LEFT);
        generator.AddLobe(CreateSphere(0.0, 2.0, 0.0), LEFT);
        generator.AddLobe(CreateSphere(2.0, 0.0, 0.0), RIGHT);

        generator.AssignGrowthApices();

        generator.DistributePoints();

        //Check that the correct number of points have been distributed
        typedef std::pair<AirwayGenerator*, LungLocation> pair_type;
        for (std::vector<pair_type>::iterator iter = generator.mLobeGenerators.begin();
            iter != generator.mLobeGenerators.end();
            ++iter)
        {
            if (iter->second == LEFT) //Two lung lobes, therefore half the number of points are expected
            {
                TS_ASSERT_DELTA(iter->first->GetPointCloud()->GetNumberOfPoints(), 25, 2);
            }
            else //iter->second == RIGHT
            {
                TS_ASSERT_DELTA(iter->first->GetPointCloud()->GetNumberOfPoints(), 50, 2);
            }
        }

    #endif
    }

    void TestDistributePointsByVolume() throw(Exception)
    {
    #if defined(CHASTE_VTK) && ( (VTK_MAJOR_VERSION >= 5 && VTK_MINOR_VERSION >= 6) || VTK_MAJOR_VERSION >= 6)

        EXIT_IF_PARALLEL;

        //Major airways mesh has 4 end points at (+/-2,0,0) and (0,+/-2,0), two are unused
        TetrahedralMesh<1,3> airways_mesh;
        TrianglesMeshReader<1,3> airways_mesh_reader("lung/test/airway_generation/data/test_major_airways_mesh");
        airways_mesh.ConstructFromMeshReader(airways_mesh_reader);

        MultiLobeAirwayGenerator generator(airways_mesh);

        generator.SetPointVolume(4/3*M_PI/50); //nb this results in more points than the test above due to the 'spheres' being smaller than real spheres
        generator.SetNumberOfPointsPerLung(50u);
        TS_ASSERT_THROWS_CONTAINS(generator.DistributePoints(), "Both SetNumberOfPointsPerLung and SetPointVolume called. Please use one or the other.");

        generator.SetNumberOfPointsPerLung(0u);

        generator.SetMinimumBranchLength(1.2);
        generator.SetPointLimit(5);
        generator.SetAngleLimit(90.0);
        generator.SetBranchingFraction(0.5);
        generator.SetDiameterRatio(1.6);

        generator.AddLobe(CreateSphere(-2.0, 0.0, 0.0), LEFT);
        generator.AddLobe(CreateSphere(2.0, 0.0, 0.0), RIGHT);

        generator.AssignGrowthApices();

        generator.DistributePoints();

        //Check that the correct number of points have been distributed
        typedef std::pair<AirwayGenerator*, LungLocation> pair_type;
        for (std::vector<pair_type>::iterator iter = generator.mLobeGenerators.begin();
            iter != generator.mLobeGenerators.end();
            ++iter)
        {
            if (iter->second == LEFT) //Two lung lobes, therefore half the number of points are expected
            {
                TS_ASSERT_DELTA(iter->first->GetPointCloud()->GetNumberOfPoints(), 50, 6);
            }
            else //iter->second == RIGHT
            {
                TS_ASSERT_DELTA(iter->first->GetPointCloud()->GetNumberOfPoints(), 50, 6);
            }
        }

    #endif
    }

    void TestGenerate() throw(Exception)
    {
#if defined(CHASTE_VTK) && ( (VTK_MAJOR_VERSION >= 5 && VTK_MINOR_VERSION >= 6) || VTK_MAJOR_VERSION >= 6)

        EXIT_IF_PARALLEL;

        TetrahedralMesh<1,3> airways_mesh;
        TrianglesMeshReader<1,3> airways_mesh_reader("lung/test/airway_generation/data/test_major_airways_mesh");
        airways_mesh.ConstructFromMeshReader(airways_mesh_reader);

        MultiLobeAirwayGenerator generator(airways_mesh);
        generator.SetNumberOfPointsPerLung(8);
        generator.SetMinimumBranchLength(0.00001);
        generator.SetPointLimit(1);
        generator.SetAngleLimit(180.0);
        generator.SetBranchingFraction(0.5);
        generator.SetDiameterRatio(1.6);

        generator.AddLobe(CreateCube(-2.9, 0.0, 0.0), LEFT);
        generator.AddLobe(CreateCube(0.0, 2.9, 0.0), LEFT);
        generator.AddLobe(CreateCube(2.9, 0.0, 0.0), RIGHT);

        generator.AssignGrowthApices();
        generator.DistributePoints();

        airways_mesh.GetNode(0)->rGetNodeAttributes()[1] = 5u;
        TS_ASSERT_THROWS_CONTAINS(generator.Generate("TestMultiLobeAirwayGenerator", "composite"),
                                  "The second node attribute in the major airways mesh ");

        airways_mesh.GetNode(0)->rGetNodeAttributes()[1] = 0u;

        generator.Generate("TestMultiLobeAirwayGenerator", "composite");

        // Check the final mesh
        TetrahedralMesh<1,3> composite_mesh;
        OutputFileHandler file_handler("TestMultiLobeAirwayGenerator", false);
        TrianglesMeshReader<1,3> composite_mesh_reader(file_handler.GetOutputDirectoryFullPath() + "composite");
        composite_mesh.ConstructFromMeshReader(composite_mesh_reader);

        //Check that the correct number of branches have been created
        TS_ASSERT_EQUALS(composite_mesh.GetNumElements(), composite_mesh.GetNumNodes() - 1);

        //Check that nodes have been tagged correctly. (2.0 == major, 1.0 == transitional, 0.0 == generated)
        TS_ASSERT_DELTA(composite_mesh.GetNode(0)->rGetNodeAttributes()[1], 2.0, 1e-6);
        TS_ASSERT_DELTA(composite_mesh.GetNode(1)->rGetNodeAttributes()[1], 2.0, 1e-6);
        TS_ASSERT_DELTA(composite_mesh.GetNode(2)->rGetNodeAttributes()[1], 1.0, 1e-6);
        TS_ASSERT_DELTA(composite_mesh.GetNode(3)->rGetNodeAttributes()[1], 1.0, 1e-6);
        TS_ASSERT_DELTA(composite_mesh.GetNode(4)->rGetNodeAttributes()[1], 1.0, 1e-6);
        TS_ASSERT_DELTA(composite_mesh.GetNode(5)->rGetNodeAttributes()[1], 1.0, 1e-6);
        TS_ASSERT_DELTA(composite_mesh.GetNode(6)->rGetNodeAttributes()[1], 0.0, 1e-6);
        TS_ASSERT_DELTA(composite_mesh.GetNode(7)->rGetNodeAttributes()[1], 0.0, 1e-6);

        ///\todo Check radii etc

    #endif
    }

    void TestDummyClassCoverage()
    {
#if !(defined(CHASTE_VTK) && ( (VTK_MAJOR_VERSION >= 5 && VTK_MINOR_VERSION >= 6) || VTK_MAJOR_VERSION >= 6))
       EXIT_IF_PARALLEL;

       MultiLobeAirwayGenerator generator;

    #endif
    }

private:
#if defined(CHASTE_VTK) && ( (VTK_MAJOR_VERSION >= 5 && VTK_MINOR_VERSION >= 6) || VTK_MAJOR_VERSION >= 6)
    vtkSmartPointer<vtkPolyData> CreateSphere(double XCentre, double YCentre, double ZCentre)
    {
        vtkSmartPointer<vtkSphereSource> sphere = vtkSmartPointer<vtkSphereSource>::New();
        sphere->SetCenter(XCentre, YCentre, ZCentre);
        sphere->SetRadius(1.0);
        sphere->SetThetaResolution(18);
        sphere->SetPhiResolution(18);
        sphere->Update();

        vtkSmartPointer<vtkPolyData> sphere_data = sphere->GetOutput();
        return sphere_data;
    }
#endif

#if defined(CHASTE_VTK) && ( (VTK_MAJOR_VERSION >= 5 && VTK_MINOR_VERSION >= 6) || VTK_MAJOR_VERSION >= 6)
    vtkSmartPointer<vtkPolyData> CreateCube(double XCentre, double YCentre, double ZCentre)
    {
        vtkSmartPointer<vtkCubeSource> cube = vtkSmartPointer<vtkCubeSource>::New();
        double size = 1.0;
        cube->SetBounds(XCentre - size, XCentre + size,
                        YCentre - size, YCentre + size,
                        ZCentre - size, ZCentre + size);

        vtkSmartPointer<vtkTriangleFilter> triangle_filter =
          vtkSmartPointer<vtkTriangleFilter>::New();
        triangle_filter->SetInputConnection(cube->GetOutputPort());
        triangle_filter->Update();

        return triangle_filter->GetOutput();
    }
#endif
};

#endif /* TESTMULTILOBEAIRWAYGENERATOR_HPP_ */
