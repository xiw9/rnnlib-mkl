/*Copyright 2009,2010 Alex Graves

This file is part of RNNLIB.

RNNLIB is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RNNLIB is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RNNLIB.  If not, see <http://www.gnu.org/licenses/>.*/

#include <boost/iostreams/tee.hpp>
#include <boost/iostreams/stream.hpp>
#include "MultilayerNet.hpp"
#include "NetcdfDataset.hpp"
#include "GradientCheck.hpp"
#include "WeightContainer.hpp"
#include "SteepestDescent.hpp"
#include "Rprop.hpp"
#include "Trainer.hpp"

typedef boost::iostreams::tee_device<ostream, ofstream> TeeDev;
typedef boost::iostreams::stream<TeeDev> TeeStream;
		
extern bool verbose;
vector<string> validDatasets = list_of<string>("train")("test")("val");

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		cout << "usage rnnlib [config_options] config_file" << endl;
		cout << "config_options syntax: --<variable_name>=<variable_value>" << endl;
		cout << "whitespace not allowed in variable names or values" << endl;
		cout << "all config_file variables overwritten by config_options" << endl;
		cout << "setting <variable_value> = \"\" removes the variable from the config" << endl;
		cout << "repeated variables overwritten by last specified" << endl;
		exit(0);
	}
	ConfigFile conf(argv[argc - 1]);
	LOOP(int arg, span(1, argc - 1))
	{
		vector<string> argument = split<string>(argv[arg], '=', 2);
		check(argument[0].substr(0, 2) == "--", "invalid option name " + argument[0]);
		string varName = argument[0].substr(2);
		if (argument.size() < 2 || argument[1] == "\"\"")
		{
			conf.remove(varName);
		}
		else
		{
			conf.set_val<string>(varName, argument[1], false);
		}
	}
	bool autosave = conf.get<bool>("autosave", false);
	string configFilename;
#ifdef FAST_LOGISTIC
	Logistic::fill_lookup();
#endif
	string task = conf.get<string>("task");
	if (task == "prediction" && conf.get<int>("predictionSteps", 1) == 1)
	{
		task = conf.set_val<string>("task", "window-prediction");
	}
	bool display = conf.get<bool>("display", false);
	vector<int> jacobianCoords = conf.get_list<int>("jacobianCoords");
	bool gradCheck = conf.get<bool>("gradCheck", false);
	verbose = conf.get<bool>("verbose", false);
	int displaySequence = conf.get<int>("sequence", 0);
	string dataset = conf.get<string>("dataset", "train");
	check(in(validDatasets, dataset), 
		  dataset + " given as 'dataset' parameter in config file '" 
		  + configFilename + "'\nmust be one of '" + str(validDatasets) + "'");
	string dataFileString = dataset + "File";
	string saveName = "";
	ofstream* logout = 0;
	TeeDev* tdev = 0;
	TeeStream* tout = 0;
	string dumpPath = "";
	string logname = "";
	if (display || jacobianCoords.size())
	{
		dumpPath = conf.get<string>("dumpPath");
		logname = dumpPath + "log";
	} 
	else if (autosave)
	{
		if (in (conf.filename, '@'))
		{
			saveName = conf.filename.substr(0, conf.filename.find('@'));
		}
		else
		{
			saveName = conf.filename.substr(0, conf.filename.rfind('.'));
		}
		saveName += "@" + time_stamp();
		logname = saveName + ".log";
	}
	if (autosave || display || jacobianCoords.size())
	{
		logout = new ofstream(logname.c_str());
		check(logout->is_open(), "can't open log file " + logname);
		tdev = new TeeDev(cout, *logout);
		tout = new TeeStream(*tdev);
		cout << "writing to log file " << logname << endl;
	}
	ostream& out = tout ? *tout : cout;
	vector<string> dataFiles = conf.get_list<string>(dataFileString);
	int dataFileNum = conf.get<int>("dataFileNum", 0);
	check(dataFiles.size() > dataFileNum, "no " + ordinal(dataFileNum) + " file in size " + str(dataFiles.size()) + " file list " + dataFileString + " in " + configFilename);
	string datafile = dataFiles[dataFileNum];
	DataHeader header(datafile, task, 1);
	DataSequence* testSeq = 0;
	if (display || gradCheck || jacobianCoords.size())
	{
		NetcdfDataset* data = new NetcdfDataset(datafile, task, displaySequence);
		testSeq = new DataSequence((*data)[0]);
		delete data;
	}
	Mdrnn *net;
	PRINT(task, out);
	net = new MultilayerNet(out, conf, header);

	//build weight container after net is created
	WeightContainer& wc =  WeightContainer::instance();
	wc.build();
	int numWeights = WeightContainer::instance().weights.size();
	
	//build the network after the weight container
	net->build();
	
	//print out network
	out << endl << "network:" << endl;
	PRINT(task, out);
	out << *net;
	out << numWeights << " weights" << endl << endl;

	//create trainer
	Trainer trainer(out, net, conf);
	out << "setting random seed to " << Random::set_seed(conf.get<unsigned long int>("randSeed", 0)) << endl << endl;
	if (conf.get<bool>("loadWeights", false))
	{
		out << "loading dynamic data from "  << conf.filename << endl;
		DataExportHandler::instance().load(conf, out);
		out << "epoch = " << trainer.epoch << endl << endl;
	}
	real_t initWeightRange = conf.get<real_t>("initWeightRange", 0.1);
	int numRandWts = wc.randomise(initWeightRange);
	if (numRandWts)
	{
		out << numRandWts << " uninitialised weights randomised uniformly in [-" << initWeightRange << "," << initWeightRange << "]" << endl;
	}
	if (testSeq && conf.get<bool>("testDistortions", false) && trainer.print_distortions())
	{
		*testSeq = *trainer.apply_distortions(testSeq);
	}
	if (gradCheck)
 	{
		out << "data header:" << endl << header << endl;
		out << *testSeq; 
		prt_line(out);
 		GradientCheck(out, net, *testSeq, conf.get<int>("sigFigs", 6), 
			conf.get<real_t>("pert", 1e-5), conf.get<bool>("verbose", false), 
			conf.get<bool>("breakOnError", true));
		conf.warn_unused(out);
 	}
	else if (jacobianCoords.size())
	{
		PRINT(dataset, out);
		PRINT(datafile, out);
		out << endl;
		out << "data header:" << endl << header << endl;
		out << "calculating Jacobian for sequence " << displaySequence << " at coords " << jacobianCoords << endl;
		out << *testSeq; 
		out << "output path: " << endl << dumpPath << endl;
		net->feed_forward(*testSeq);
		net->print_output_shape(out);
		Layer* outputLayer = net->outputLayers.size() ? net->outputLayers.front() : net->hiddenLayers.back();
		int D = outputLayer->num_seq_dims() + 1;
		check((jacobianCoords.size() == D) || (jacobianCoords.size() == (D - 1)), "Jacobian coords length " + str(jacobianCoords.size()) + " for output layer depth " + str(D));
		if (jacobianCoords.size() == D)
		{
			outputLayer->outputErrors.get(jacobianCoords) = outputLayer->outputActivations.get(jacobianCoords);
		}
		else
		{
			outputLayer->outputErrors[jacobianCoords] = outputLayer->outputActivations[jacobianCoords];
		}	
		net->feed_back();
		DataExportHandler::instance().display(dumpPath);
		conf.warn_unused(out);
	}
	else if (display)
	{
		out << "data header:" << endl << header << endl;
		out << "displaying sequence " << displaySequence << endl;
		out << *testSeq; 
		out << "output path: " << endl << dumpPath << endl;
		net->train(*testSeq);
		net->print_output_shape(out);
		out << "errors:" << endl << net->errors;
		DataExportHandler::instance().display(dumpPath);
		conf.warn_unused(out);
	}
	else if (conf.get<bool>("errorTest", false))
	{
		trainer.calculate_all_errors();
		conf.warn_unused(out);
	}
	else
	{
		out << "trainer:" << endl;
		trainer.train(saveName);
	}
	if (testSeq)
	{
		delete testSeq;
	}
	if (logout)
	{
		delete logout;
	}
	if (tdev)
	{
		delete tdev;
	}
//	if (tout)
//	{
//		delete tout;
//	}
	delete net;
}
