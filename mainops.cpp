 /*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) <year>  <name of author>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
    Author : Jose Fernandez Navarro  -  jc.fernandez.navarro@gmail.com
*/

#include "mainops.h"
#include "phyltr.h"
#include <boost/dynamic_bitset.hpp>
#include "libraries/Node.hh"
#include "libraries/TreeIO.hh"
#include "libraries/AnError.hh"
#include "DrawTree_time.hh"
#include "libraries/gammamapex.h"
#include "libraries/lambdamapex.h"
#include <vector>
#include "layoutrees.h"
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
using namespace std;

Mainops::Mainops()
 :Guest(0),Host(0),gamma(0),lambdamap(0),dt(0),late(0),parameters(0),io(0)
{

}

Mainops::~Mainops()
{
  delete(Guest);
  delete(Host);
  delete(gamma);
  delete(lambdamap);
  delete(dt);
  delete(late);
  delete(io);
  if(layout) delete(layout);
}

bool Mainops::lateralTransferDP(string mapname)
{  
      late = new Phyltr();
      late->g_input.duplication_cost = parameters->lateralduplicost;
      late->g_input.transfer_cost = parameters->lateraltrancost;
      late->g_input.max_cost = parameters->lateralmaxcost;
      late->g_input.min_cost = parameters->lateralmincost;
      late->g_input.print_only_minimal_loss_scenarios = false;
      late->g_input.print_only_minimal_transfer_scenarios = false;
      late->g_input.gene_tree = Guest;
      late->g_input.species_tree = Host;
      
      if(parameters->isreconciled)
      {
	late->g_input.sigma_fname = mapname;
	late->read_sigma();
      }
      else
      {
	late->read_sigma(gs.getMapping());
      }
      
      late->dp_algorithm();
      late->backtrack();
      
      if(late->scenarios.size() > 0 && thereAreLGT(late->scenarios))
      {
	Scenario optscenario = late->getMinCostScenario(); 
	lambda = optscenario.cp.getLambda();
	transferedges = optscenario.transfer_edges;
	parameters->transferedges = transferedges;
	sigma = late->g_input.sigma;
	scenarios = late->scenarios;
	
	return true;
      }
      else
      {
	parameters->lattransfer = false;
	return false;
      }

}

bool Mainops::lateralTransfer(string mapname)
{
      late = new Phyltr();
      late->g_input.duplication_cost = parameters->lateralduplicost;
      late->g_input.transfer_cost = parameters->lateraltrancost;
      late->g_input.max_cost = parameters->lateralmaxcost;
      late->g_input.min_cost = parameters->lateralmincost;
      late->g_input.gene_tree = Guest;
      late->g_input.species_tree = Host;
      
      if(parameters->isreconciled)
      {
	late->g_input.sigma_fname = mapname;
	late->read_sigma();
      }
      else
      {
	late->read_sigma(gs.getMapping());
      }
      
      late->fpt_algorithm();
      
      if(late->scenarios.size() > 0 && thereAreLGT(late->scenarios))
      {
	Scenario optscenario = late->getMinCostScenario(); 
	lambda = optscenario.cp.getLambda();
	transferedges = optscenario.transfer_edges;
	parameters->transferedges = transferedges;
	sigma = late->g_input.sigma;
	scenarios = late->scenarios;
	
	return true;
      }
      else
      {
	parameters->lattransfer = false;
	return false;
      }
}

void Mainops::printLGT()
{
  BOOST_FOREACH(Scenario &sc, scenarios)
  {
    std::cout << "\n" << sc << std::endl;
    std::cout << "Lambda" << std::endl;
    for(std::vector<unsigned>::const_iterator it = sc.cp.getLambda().begin();
	it != sc.cp.getLambda().end(); ++it)
	  std::cout << *it << " ";
    std::cout << "\nSigma" << std::endl;
    for(std::vector<unsigned>::const_iterator it = late->g_input.sigma.begin();
	it != late->g_input.sigma.end(); ++it)
	  std::cout << *it << " ";
  }
}


bool Mainops::thereAreLGT(std::vector<Scenario> scenarios)
{
  BOOST_FOREACH(Scenario &sc, scenarios)
  {
    if(sc.transfer_edges.any())
      return true;
  }
  return false;
}


void Mainops::OpenReconciled(const char* reconciled)
{
     
    io = new TreeIO(TreeIO::fromFile(reconciled));
     
    Guest = new TreeExtended(io->readBeepTree<TreeExtended,Node>(&AC, &gs));

}

void Mainops::OpenHost(const char* species)
{
    io->setSourceFile(species);
    io->checkTagsForTree(traits);
 
    if(traits.containsTimeInformation() == false)
 	throw AnError("Host tree lacks time information for some of it nodes", 1);
    else
 	traits.enforceHostTree();

    Host = new TreeExtended(io->readBeepTree<TreeExtended,Node>(traits,0,0));
    Node *root = Host->getRootNode();

    if (root->getTime() == 0.0)
    {
      Real t = root->getNodeTime();
      root->setTime(0.1 * t); //the assert inside the function is failing
    }

    if (Host->imbalance() / Host->rootToLeafTime() > 0.01) {
 	parameters->scaleByTime = false;
 	cerr << "The species tree is not ultrametric (it appears unbalanced),\n"
 	  "so scaling by time is turned off. See also option '-t'.\n";

    }
    
}

void Mainops::CalculateGamma()
{

    if (parameters->do_not_draw_species_tree == false)
    {
	gamma = new  GammaMapEx<Node>(*Guest, *Host, gs, AC);
	
	//NOTE I could use the lambda estimated by Phyltr
        if (parameters->lattransfer)
        {
	  gamma = new GammaMapEx<Node>(gamma->update(*Guest,*Host,sigma,transferedges));
        }
    }
    else
    {
       //NOTE I could use the lambda estimated by Phyltr
	lambdamap = new LambdaMapEx<Node>(*Guest, *Host, gs);

        if (parameters->lattransfer)
	{
            lambdamap->update(*Guest,*Host,sigma,transferedges);
	}
	
	gamma = new GammaMapEx<Node>(GammaMapEx<Node>::MostParsimonious(*Guest,*Host,*lambdamap));
    }

  
}


void Mainops::reconcileTrees(const char* gene, const char* species, const char* mapfile)
{

    io = new TreeIO(TreeIO::fromFile(gene));
    Guest = new TreeExtended(io->readBeepTree<TreeExtended,Node>(&AC, &gs));
    
    io->setSourceFile(species);
    Host = new TreeExtended(io->readNewickTree<TreeExtended,Node>());

    if (strcmp(mapfile,"")!=0)
    {  
      gs = TreeIO::readGeneSpeciesInfo(mapfile);
    }

    lambdamap = new LambdaMapEx<Node>(*Guest, *Host, gs);

    gamma = new GammaMapEx<Node>(GammaMapEx<Node>::MostParsimonious(*Guest, *Host, *lambdamap));
    
    string textTree =  io->writeGuestTree<TreeExtended,Node>(*Guest,gamma);
    
    delete(io);
    io = new TreeIO(TreeIO::fromString(textTree));
   
    delete(Guest);
    Guest = new TreeExtended(io->readBeepTree<TreeExtended,Node>(&AC, &gs));

    OpenHost(species);

}



 void Mainops::calculateCordinates()
  {
     bool status = true;

     if(parameters->lattransfer)
       status = getValidityLGT();
     else
       status = gamma->valid();
     
     if (!status) {
	if(parameters->lattransfer)
	  throw AnError(": The LGT scenario was not valid. Aborts!\n");
	else
	  throw AnError(": This is not a correctly reconciled tree. Aborts!\n");
     }  
     else 
     {   
        Host->reset();
	Guest->reset();
	LayoutTrees *spcord = new LayoutTrees(*Host,*Guest,*parameters,*gamma);   
	//reduce crossing only if not LGT 
	if(parameters->reduce && !parameters->lattransfer)
	{
	  layout = new Layout(Host->getRootNode(), *(Guest->getRootNode()), *gamma);
	  /* I should get a map here */
	  //spcord->replaceNodes(map)
	  delete layout;
	}
	parameters->leafwidth = spcord->getNodeHeight();

     }
  }


void Mainops::DrawTree(cairo_t *cr)
{
    dt = new DrawTree_time(*parameters,*Guest,*Host,*gamma,cr);
    
    //TODO here I should calculate transformation according to fixed crossing lines as
    // in GDrawConstraints in Marco's code
    dt->calculateTransformation();
    
    if(parameters->do_not_draw_species_tree == false) {
       //species tree
      if (!parameters->noTimeAnnotation) {
	if (parameters->timeAtEdges) {
	    dt->TimeLabelsOnEdges();
	} else {
	    dt->DrawTimeEdges();
	    dt->DrawTimeLabels();
	}
      }
      dt->DrawSpeciesEdgesWithContour();
      dt->DrawSpeciesNodes();
      dt->DrawSpeciesNodeLabels();
    }
    // gene tree
    if(!parameters->do_not_draw_guest_tree)
    {
      dt->DrawGeneEdges();
      dt->DrawGeneNodes();
      dt->DrawGeneLabels();
      if(parameters->markers)
	dt->GeneTreeMarkers();
    }
    
    if(parameters->header)
      dt->createHeader();
    if(parameters->legend)
      dt->createLegend();
    if(parameters->title)
      dt->createTitle();
    if (parameters->show_event_count)
      dt->writeEventCosts();
}


int Mainops::RenderImage()
{
  return dt->RenderImage();
}

Parameters* Mainops::getParameters()
{
  return parameters;
}

void Mainops::setParameters(Parameters *p)
{
  parameters = p;
}

bool Mainops::getValidityLGT()
{
  if(gamma->validLGT())
    return true;
  else
  {
    sort(scenarios.begin(), scenarios.end());
  
    BOOST_FOREACH (Scenario &sc, scenarios)
    {
      transferedges = sc.transfer_edges;
      parameters->transferedges = sc.transfer_edges;
      parameters->duplications = sc.duplications;
      lambda = sc.cp.getLambda();
      CalculateGamma();
      if(gamma->validLGT())
      { 
	return true;
      }
    }
    
    return false;
  } 
}

void Mainops::drawBest()
{
   CalculateGamma(); //calculation of gamma and lambda      
   calculateCordinates(); //calculation of the drawing cordinates
   DrawTree();  //drawing the tree
   RenderImage(); // save the file
}

void Mainops::drawAllLGT()
{
  unsigned index = 0;
  std::string original_filename = parameters->outfile;
  sort(scenarios.begin(), scenarios.end());
  BOOST_FOREACH (Scenario &sc, scenarios)
  {
    transferedges = sc.transfer_edges;
    parameters->transferedges = sc.transfer_edges;
    parameters->duplications = sc.duplications;
    lambda = sc.cp.getLambda();
    CalculateGamma();
    if(gamma->validLGT())
    { 
      //NOTE added afer file extension??
      parameters->outfile = original_filename + boost::lexical_cast<string>(++index);
      drawBest();
    }
  }
}

void Mainops::loadPreComputedScenario(const std::string &filename)
{
  
  std::ifstream scenario_file;
  std::string line;
  scenario_file.open(filename.c_str(), std::ios::in);
  if (!scenario_file) 
  {
     throw AnError("Could not open file " + filename);
  }
  
  std::vector<unsigned> sigma_temp;
  
  while (getline(scenario_file, line)) 
  {
     if(scenario_file.good())    
     {
        if(line.size() == 0)  // Skip any blank lines
                continue;
        else if(line[0] == '#')  // Skip any comment lines
                continue;
        else
	{
	  if ((line.find("Transfer") != std::string::npos))
	  {
	    const std::size_t start_pos = line.find(":");
	    const std::size_t stop_pos = line.size() - 1;
	    std::string temp = line.substr(start_pos + 1,stop_pos - start_pos);
	    std::cout << "Reading lateral transfer " << temp << std::endl;
	    temp.erase(remove(temp.begin(),temp.end(),' '),temp.end());
	    stringstream lineStream(temp);
	    std::vector<unsigned> transfer_nodes((istream_iterator<int>(lineStream)), istream_iterator<int>());
	    std::cout << "Reading lateral transfer vector size " << transfer_nodes.size() << std::endl;
	    transferedges.clear();
	    transferedges.resize(Guest->getNumberOfNodes());
	    for(std::vector<unsigned>::const_iterator it = transfer_nodes.begin(); it != transfer_nodes.end(); ++it)
	      transferedges.set(boost::lexical_cast<unsigned>(*it));
	  }
	  else if ((line.find("Lambda") != std::string::npos))
	  {
	    const std::size_t start_pos = line.find_first_of(":");
	    const std::size_t stop_pos = line.size() - 1;
	    std::string temp = line.substr(start_pos + 1,stop_pos - start_pos);
	    std::cout << "Reading Lambda " << temp << std::endl;
	    temp.erase(remove(temp.begin(),temp.end(),' '),temp.end());
	    unsigned mapped_lambda_temp = boost::lexical_cast<unsigned>(temp);
	    sigma_temp.push_back(mapped_lambda_temp);
	    std::cout << "Reading Lambda vector size " << sigma_temp.size() << std::endl;
	  }
	  else
	  {
	    continue;
	  }
      }
     
    }
  }
  scenario_file.close();
  sigma = sigma_temp;
  CalculateGamma();
  if(gamma->validLGT())
  {
    drawBest();
  }
}