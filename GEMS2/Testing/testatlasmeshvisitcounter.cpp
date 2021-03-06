#include <boost/test/unit_test.hpp>

#include "itkImageRegionConstIteratorWithIndex.h"

#include "kvlAtlasMesh.h"
#include "atlasmeshvisitcounter.hpp"
#include "atlasmeshvisitcountercpuwrapper.hpp"
#ifdef CUDA_FOUND
#include "atlasmeshvisitcountercuda.hpp"
#endif

#include "testfileloader.hpp"

// --------------------

void CheckVisitCounter( kvl::interfaces::AtlasMeshVisitCounter* visitCounter,
			TestFileLoader::ImageType::ConstPointer targetImage,
			kvl::AtlasMesh::ConstPointer targetMesh ) {
  kvl::AtlasMeshVisitCounterCPU::Pointer  originalVisitCounter = kvl::AtlasMeshVisitCounterCPU::New();

  originalVisitCounter->SetRegions( targetImage->GetLargestPossibleRegion() );
  originalVisitCounter->Rasterize( targetMesh );
  BOOST_TEST_MESSAGE("Original counter complete");
  
  visitCounter->SetRegions( targetImage->GetLargestPossibleRegion() );
  visitCounter->VisitCount( targetMesh );
  BOOST_TEST_MESSAGE("AtlasMeshVisitCounterCPUWrapper complete");
  
  itk::ImageRegionConstIteratorWithIndex<kvl::interfaces::AtlasMeshVisitCounter::ImageType>  
    it( visitCounter->GetImage(), visitCounter->GetImage()->GetBufferedRegion() );
  itk::ImageRegionConstIteratorWithIndex<kvl::AtlasMeshVisitCounterCPU::ImageType>  
    itOrig( originalVisitCounter->GetImage(), originalVisitCounter->GetImage()->GetBufferedRegion() );
  
  for( ; !it.IsAtEnd(); ++it, ++itOrig ) {
    BOOST_TEST_CONTEXT( "Voxel Index: " << it.GetIndex() ) {
      BOOST_CHECK_EQUAL( it.Value(), itOrig.Value() );
    }
  }
}

// -------------------

typedef kvl::interfaces::AtlasMeshVisitCounter::ImageType ImageType;
typedef itk::AutomaticTopologyMeshSource<kvl::AtlasMesh> MeshSource;
typedef MeshSource::IdentifierType  IdentifierType;
typedef kvl::AtlasMesh Mesh;

const int nDims = 3;
const int nVertices = 4;

ImageType::Pointer CreateImageCube( const int sideLength, const int value ) {
  const int nx = sideLength;
  const int ny = sideLength;
  const int nz = sideLength;

  ImageType::RegionType region;
  ImageType::IndexType start;
  ImageType::SizeType size;
  start[0] = start[1] = start[2] = 0;
  size[0] = nx;
  size[1] = ny;
  size[2] = nz;

  region.SetSize(size);
  region.SetIndex(start);

  ImageType::Pointer image = ImageType::New();
  image->SetRegions(region);
  image->Allocate();

  for( int k=0; k<nz; k++ ) {
    for( int j=0; j<ny; j++ ) {
      for( int i=0; i<nx; i++ ) {
	ImageType::IndexType idx;
	idx[0] = i;
	idx[1] = j;
	idx[2] = k;

	image->SetPixel(idx, value);
      }
    }
  }

  return image;
}

Mesh::Pointer CreateSingleTetrahedronMesh( float vertices[nVertices][nDims] ) {
  MeshSource::Pointer meshSource = MeshSource::New();

  const IdentifierType  id0 = meshSource->AddPoint( vertices[0] );
  const IdentifierType  id1 = meshSource->AddPoint( vertices[1] );
  const IdentifierType  id2 = meshSource->AddPoint( vertices[2] );
  const IdentifierType  id3 = meshSource->AddPoint( vertices[3] );
  meshSource->AddTetrahedron( id0, id1, id2, id3 );

  return meshSource->GetOutput();
}

// -------------------

BOOST_AUTO_TEST_SUITE( AtlasMeshVisitCounter )

BOOST_AUTO_TEST_SUITE( UnitCubeSingleTetrahedron )

BOOST_AUTO_TEST_CASE( LowerCorner )
{
  const int imageSize = 2;
  const int nx = imageSize;
  const int ny = imageSize;
  const int nz = imageSize;

  ImageType::Pointer image = CreateImageCube( imageSize, 0 );
  BOOST_TEST_CHECKPOINT("Image created");

  // Define tetrahedron enclosing origin and (0,0,1), (0,1,0) and (1,0,0)
  const float delta = 0.1f;
  BOOST_REQUIRE( delta < 0.5f );
  float verts[nVertices][nDims] = {
    { -delta, -delta, -delta },
    { 1+(2*delta), -delta, -delta },
    { -delta, 1+(2*delta), -delta },
    { -delta, -delta, 1+(2*delta) }
  };

  Mesh::Pointer mesh = CreateSingleTetrahedronMesh( verts );
  BOOST_TEST_CHECKPOINT("Mesh created");

  kvl::AtlasMeshVisitCounterCPU::Pointer visitCounter = kvl::AtlasMeshVisitCounterCPU::New();

  visitCounter->SetRegions( image->GetLargestPossibleRegion() );
  visitCounter->Rasterize( mesh );

  BOOST_TEST_CHECKPOINT("VisitCounter Complete");

  // Check points in tetrahedron
  const ImageType* result = visitCounter->GetImage();
  for( int k=0; k<nz; k++ ) {
    for( int j=0; j<ny; j++ ) {
      for( int i=0; i<nx; i++ ) {
	ImageType::IndexType idx;
	idx[0] = i;
	idx[1] = j;
	idx[2] = k;

	// Recall that the tetrahedron is constructed to be the lower one
	// enclosing points with at most one index equal to 1
	int expected = 0;
	if( i+j+k <= 1 ) {
	  expected = 1;
	}

	BOOST_TEST_INFO( "(" << i << "," << j << "," << k << ")" );
	BOOST_CHECK_EQUAL( result->GetPixel(idx), expected );
      }
    }
  }
}

BOOST_AUTO_TEST_CASE(OriginOnly)
{
  const int imageSize = 2;
  const int nx = imageSize;
  const int ny = imageSize;
  const int nz = imageSize;

  ImageType::Pointer image = CreateImageCube( imageSize, 0 );
  BOOST_TEST_CHECKPOINT("Image created");

  // Define tetrahedron enclosing origin only
  const float delta = 0.1f;
  BOOST_REQUIRE( delta < 0.5f );
  float verts[nVertices][nDims] = {
    { -delta, -delta, -delta },
    { delta, 0, 0 },
    { 0, delta, 0 },
    { 0, 0, delta }
  };

  Mesh::Pointer mesh = CreateSingleTetrahedronMesh( verts );
  BOOST_TEST_CHECKPOINT("Mesh created");

  kvl::AtlasMeshVisitCounterCPU::Pointer visitCounter = kvl::AtlasMeshVisitCounterCPU::New();

  visitCounter->SetRegions( image->GetLargestPossibleRegion() );
  visitCounter->Rasterize( mesh );

  BOOST_TEST_CHECKPOINT("VisitCounter Complete");

  // Check points in tetrahedron
  const ImageType* result = visitCounter->GetImage();
  for( int k=0; k<nz; k++ ) {
    for( int j=0; j<ny; j++ ) {
      for( int i=0; i<nx; i++ ) {
	ImageType::IndexType idx;
	idx[0] = i;
	idx[1] = j;
	idx[2] = k;

	int expected = 0;
	if( i+j+k == 0 ) {
	  expected = 1;
	}

	BOOST_TEST_INFO( "(" << i << "," << j << "," << k << ")" );
	BOOST_CHECK_EQUAL( result->GetPixel(idx), expected );
      }
    }
  }
}

BOOST_AUTO_TEST_CASE(XAxisOnly)
{
  const int imageSize = 2;
  const int nx = imageSize;
  const int ny = imageSize;
  const int nz = imageSize;

  ImageType::Pointer image = CreateImageCube( imageSize, 0 );
  BOOST_TEST_CHECKPOINT("Image created");

  // Define tetrahedron enclosing origin and (1,0,0)
  const float delta = 0.1f;
  BOOST_REQUIRE( delta < 0.5f );
  float verts[nVertices][nDims] = {
    { -delta, -delta, -delta },
    { 1+(2*delta), 0, 0 },
    { -delta, 2*delta, -delta },
    { -delta, -delta, 2*delta }
  };

  Mesh::Pointer mesh = CreateSingleTetrahedronMesh( verts );
  BOOST_TEST_CHECKPOINT("Mesh created");

  kvl::AtlasMeshVisitCounterCPU::Pointer visitCounter = kvl::AtlasMeshVisitCounterCPU::New();

  visitCounter->SetRegions( image->GetLargestPossibleRegion() );
  visitCounter->Rasterize( mesh );

  BOOST_TEST_CHECKPOINT("VisitCounter Complete");

  // Check points in tetrahedron
  const ImageType* result = visitCounter->GetImage();
  for( int k=0; k<nz; k++ ) {
    for( int j=0; j<ny; j++ ) {
      for( int i=0; i<nx; i++ ) {
	ImageType::IndexType idx;
	idx[0] = i;
	idx[1] = j;
	idx[2] = k;

	int expected = 0;
	if( j+k == 0 ) {
	  expected = 1;
	}

	BOOST_TEST_INFO( "(" << i << "," << j << "," << k << ")" );
	BOOST_CHECK_EQUAL( result->GetPixel(idx), expected );
      }
    }
  }
}

BOOST_AUTO_TEST_CASE(FarCornerOnly)
{
  const int imageSize = 2;
  const int nx = imageSize;
  const int ny = imageSize;
  const int nz = imageSize;

  ImageType::Pointer image = CreateImageCube( imageSize, 0 );
  BOOST_TEST_CHECKPOINT("Image created");

  // Define tetrahedron enclosing (1,1,1)
  const float delta = 0.1f;
  BOOST_REQUIRE( delta < 0.5f );
  float verts[nVertices][nDims] = {
    { 1-delta, 1-delta, 1-delta },
    { 1+delta, 1, 1 },
    { 1, 1+delta, 1 },
    { 1, 1, 1+delta }
  };

  Mesh::Pointer mesh = CreateSingleTetrahedronMesh( verts );
  BOOST_TEST_CHECKPOINT("Mesh created");

  kvl::AtlasMeshVisitCounterCPU::Pointer visitCounter = kvl::AtlasMeshVisitCounterCPU::New();

  visitCounter->SetRegions( image->GetLargestPossibleRegion() );
  visitCounter->Rasterize( mesh );

  BOOST_TEST_CHECKPOINT("VisitCounter Complete");

  // Check points in tetrahedron
  const ImageType* result = visitCounter->GetImage();
  for( int k=0; k<nz; k++ ) {
    for( int j=0; j<ny; j++ ) {
      for( int i=0; i<nx; i++ ) {
	ImageType::IndexType idx;
	idx[0] = i;
	idx[1] = j;
	idx[2] = k;

	int expected = 0;
	if( i+j+k == 3 ) {
	  expected = 1;
	}

	BOOST_TEST_INFO( "(" << i << "," << j << "," << k << ")" );
	BOOST_CHECK_EQUAL( result->GetPixel(idx), expected );
      }
    }
  }
}


BOOST_AUTO_TEST_CASE(UpperCorner)
{
  const int imageSize = 2;
  const int nx = imageSize;
  const int ny = imageSize;
  const int nz = imageSize;

  ImageType::Pointer image = CreateImageCube( imageSize, 0 );
  BOOST_TEST_CHECKPOINT("Image created");

  // Define tetrahedron enclosing (0,1,1), (1,0,1), (1,1,0) and (1,1,1)
  const float delta = 0.1f;
  BOOST_REQUIRE( delta < 0.5f );
  float verts[nVertices][nDims] = {
    { 1+delta, 1+delta, 1+delta },
    { -delta, 1, 1 },
    { 1, -delta, 1 },
    { 1, 1, -delta }
  };

  Mesh::Pointer mesh = CreateSingleTetrahedronMesh( verts );
  BOOST_TEST_CHECKPOINT("Mesh created");

  kvl::AtlasMeshVisitCounterCPU::Pointer visitCounter = kvl::AtlasMeshVisitCounterCPU::New();

  visitCounter->SetRegions( image->GetLargestPossibleRegion() );
  visitCounter->Rasterize( mesh );

  BOOST_TEST_CHECKPOINT("VisitCounter Complete");

  // Check points in tetrahedron
  const ImageType* result = visitCounter->GetImage();
  for( int k=0; k<nz; k++ ) {
    for( int j=0; j<ny; j++ ) {
      for( int i=0; i<nx; i++ ) {
	ImageType::IndexType idx;
	idx[0] = i;
	idx[1] = j;
	idx[2] = k;

	int expected = 0;
	if( i+j+k >= 2 ) {
	  expected = 1;
	}

	BOOST_TEST_INFO( "(" << i << "," << j << "," << k << ")" );
	BOOST_CHECK_EQUAL( result->GetPixel(idx), expected );
      }
    }
  }
}

BOOST_AUTO_TEST_CASE(NoVertices)
{
  const int imageSize = 2;
  const int nx = imageSize;
  const int ny = imageSize;
  const int nz = imageSize;

  ImageType::Pointer image = CreateImageCube( imageSize, 0 );
  BOOST_TEST_CHECKPOINT("Image created");

  // Define tetrahedron in middle of cube enclosing no vertices
  const float delta = 0.1f;
  BOOST_REQUIRE( delta < 0.5f );
  float verts[nVertices][nDims] = {
    { 0.5f+delta, 0.5f+delta, 0.5f+delta },
    { 0.5f-delta, 0.5f, 0.5f },
    { 0.5f, 0.5f-delta, 0.5f },
    { 0.5f, 0.5f, -delta }
  };

  Mesh::Pointer mesh = CreateSingleTetrahedronMesh( verts );
  BOOST_TEST_CHECKPOINT("Mesh created");

  kvl::AtlasMeshVisitCounterCPU::Pointer visitCounter = kvl::AtlasMeshVisitCounterCPU::New();

  visitCounter->SetRegions( image->GetLargestPossibleRegion() );
  visitCounter->Rasterize( mesh );

  BOOST_TEST_CHECKPOINT("VisitCounter Complete");

  // Check points in tetrahedron
  const ImageType* result = visitCounter->GetImage();
  for( int k=0; k<nz; k++ ) {
    for( int j=0; j<ny; j++ ) {
      for( int i=0; i<nx; i++ ) {
	ImageType::IndexType idx;
	idx[0] = i;
	idx[1] = j;
	idx[2] = k;

	int expected = 0;

	BOOST_TEST_INFO( "(" << i << "," << j << "," << k << ")" );
	BOOST_CHECK_EQUAL( result->GetPixel(idx), expected );
      }
    }
  }
}

BOOST_AUTO_TEST_CASE(LowerCornerExact)
{
  const int imageSize = 2;
  const int nx = imageSize;
  const int ny = imageSize;
  const int nz = imageSize;

  ImageType::Pointer image = CreateImageCube( imageSize, 0 );
  BOOST_TEST_CHECKPOINT("Image created");

  // Define tetrahedron in on exactly (0,0,0), (1,0,0), (0,1,0) and (0,0,1)
  // This is to go after some of the edge cases
  float verts[nVertices][nDims] = {
    { 0, 0, 0 },
    { 1, 0, 0 },
    { 0, 1, 0 },
    { 0, 0, 1 }
  };

  Mesh::Pointer mesh = CreateSingleTetrahedronMesh( verts );
  BOOST_TEST_CHECKPOINT("Mesh created");

  kvl::AtlasMeshVisitCounterCPU::Pointer visitCounter = kvl::AtlasMeshVisitCounterCPU::New();

  visitCounter->SetRegions( image->GetLargestPossibleRegion() );
  visitCounter->Rasterize( mesh );

  BOOST_TEST_CHECKPOINT("VisitCounter Complete");

  // Check points in tetrahedron
  const ImageType* result = visitCounter->GetImage();
  for( int k=0; k<nz; k++ ) {
    for( int j=0; j<ny; j++ ) {
      for( int i=0; i<nx; i++ ) {
	ImageType::IndexType idx;
	idx[0] = i;
	idx[1] = j;
	idx[2] = k;

	int expected = 0;
	if( i+j+k == 0 ) {
	  expected = 1;
	}

	BOOST_TEST_INFO( "(" << i << "," << j << "," << k << ")" );
	BOOST_CHECK_EQUAL( result->GetPixel(idx), expected );
      }
    }
  }
}

BOOST_AUTO_TEST_CASE(UpperCornerExact)
{
  const int imageSize = 2;
  const int nx = imageSize;
  const int ny = imageSize;
  const int nz = imageSize;

  ImageType::Pointer image = CreateImageCube( imageSize, 0 );
  BOOST_TEST_CHECKPOINT("Image created");

  // Define tetrahedron in on exactly (1,1,1), (0,1,1), (1,0,1) and (1,1,0)
  // This is to go after some of the edge cases
  float verts[nVertices][nDims] = {
    { 1, 1, 1 },
    { 0, 1, 1 },
    { 1, 0, 1 },
    { 1, 1, 0 }
  };

  Mesh::Pointer mesh = CreateSingleTetrahedronMesh( verts );
  BOOST_TEST_CHECKPOINT("Mesh created");

  kvl::AtlasMeshVisitCounterCPU::Pointer visitCounter = kvl::AtlasMeshVisitCounterCPU::New();

  visitCounter->SetRegions( image->GetLargestPossibleRegion() );
  visitCounter->Rasterize( mesh );

  BOOST_TEST_CHECKPOINT("VisitCounter Complete");

  // Check points in tetrahedron
  const ImageType* result = visitCounter->GetImage();
  for( int k=0; k<nz; k++ ) {
    for( int j=0; j<ny; j++ ) {
      for( int i=0; i<nx; i++ ) {
	ImageType::IndexType idx;
	idx[0] = i;
	idx[1] = j;
	idx[2] = k;

	int expected = 0;

	BOOST_TEST_INFO( "(" << i << "," << j << "," << k << ")" );
	BOOST_CHECK_EQUAL( result->GetPixel(idx), expected );
      }
    }
  }
}

BOOST_AUTO_TEST_SUITE_END();

// --

BOOST_FIXTURE_TEST_SUITE( ActualImage, TestFileLoader )

BOOST_AUTO_TEST_CASE( ReferenceImpl )
{
  kvl::AtlasMeshVisitCounterCPUWrapper visitCounter;
 
  // Note that image and mesh are supplied by TestFileLoader
  CheckVisitCounter( &visitCounter, image, mesh );
}

#ifdef CUDA_FOUND
BOOST_AUTO_TEST_CASE( CUDAImpl )
{
  kvl::cuda::AtlasMeshVisitCounterCUDA visitCounter;
 
  // Note that image and mesh are supplied by TestFileLoader
  CheckVisitCounter( &visitCounter, image, mesh );
}
#endif

BOOST_AUTO_TEST_SUITE_END();

BOOST_AUTO_TEST_SUITE_END();
