#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cmath>
#include <csignal>
#include <unistd.h>
#include <boost/program_options.hpp>
#include <H5Cpp.h>
#include <opencv2/opencv.hpp>

#include "plot.hpp"


namespace po = boost::program_options;


po::variables_map parse(int argc, char **argv) {
	po::options_description op("Command line options");
	op.add_options()
	("version,v", "print version string")
	("help,h", "produce this help message")
	("info,p", "print information about the input file")
	("config,c", po::value< std::vector<std::string> >(), "path of a config file");

	po::positional_options_description p;
	p.add("file", -1);

	po::options_description fop;
	fop.add_options()
	("input,i", po::value<std::string>()->required(), "path of the input file")
	("background,b", po::value<std::string>(), "path of the input background image")
	("data,d", po::value<std::string>()->implicit_value(""), "path of the output hdf5 data file")
	("movie,m", po::value<std::string>()->implicit_value(""), "path of the output video")
	("live,l", po::bool_switch(), "display live window?")
	("frame.start", po::value<int>()->default_value(1), "first frame of interest")
	("frame.stop", po::value<int>(), "last frame of interest")
	("frame.step", po::value<int>()->default_value(1), "step between frames of interest")
	("frame.count", po::value<int>(), "number of frames of interest")
	("grid.width", po::value<int>()->default_value(128), "number of horizontal grid points")
	("grid.height", po::value<int>()->default_value(64), "number of vertical grid points");
	op.add(fop);

	// Parse command line
	po::variables_map vm;
	po::store(po::command_line_parser(argc, argv).options(op).positional(p).run(), vm);

	// Parse config files
	if (vm.count("config")) {
		std::vector<std::string> files = vm["config"].as< std::vector<std::string> >();
		for (std::vector<std::string>::const_iterator i = files.begin(); i != files.end(); ++i)
		{
			std::ifstream s(i->c_str());
			po::store(po::parse_config_file(s, fop), vm);
		}
	}

	// Parse stdin
	if (!isatty(STDIN_FILENO)) {
		po::store(po::parse_config_file(std::cin, fop), vm);
	}

	if (vm.count("version")) {
		std::cout << "fishFlow v0.2 - 2014/07/15" << std::endl;
		std::exit(0);
	}
	if (vm.count("help")) {
		std::cout << "Usage: " << argv[0] << " [options] [config_file] ...";
		std::cout << std::endl << std::endl;
		std::cout << "Synopsis: Compute velocity from videos of fish schools using optical flow";
		std::cout << std::endl << std::endl;
		std::cout << op << std::endl;
		std::exit(0);
	}
	po::notify(vm);
	if (!vm.count("data") && !vm.count("movie") && !vm["live"].as<bool>() && !vm.count("info")) {
		std::cout << "Fatal: no output was specified" << std::endl;
		std::exit(-1);
	}
	return vm;
}

void frameLogic(const po::variables_map& config, int& start, int& stop, int& step, int& count, int max_count) {
	start = config["frame.start"].as<int>();
	step = config["frame.step"].as<int>();
	stop = max_count;
	count = max_count;

	if (config.count("frame.stop")) {
		stop = config["frame.stop"].as<int>();
	}
	if (config.count("frame.count")) {
		count = config["frame.count"].as<int>();
	}
	if (start == 0) {
		std::cerr << "Fatal: frame start cannot be zero (frames start at one)" << std::endl;
		std::exit(-1);
	}
	if (step < 1) {
		std::cerr << "Fatal: frame step cannot be less than one" << std::endl;
		std::exit(-1);
	}
	if (start > stop) {
		std::cerr << "Fatal: frame start cannot be after frame stop" << std::endl;
		std::exit(-1);
	}
	if (stop > max_count) {
		std::cerr << "Fatal: frame stop is past the last frame" << std::endl;
		std::exit(-1);
	}
	if ((stop - start + 1) / step != count) {
		if (config.count("frame.stop") && config.count("frame.count")) {
			std::cout << "Fatal: both frame stop and count specified" << std::endl;
			std::exit(-1);
		} else if (config.count("frame.count")) {
			stop = start + (count - 1) * step;
		} else {
			count = (stop - start + 1) / step;
		}
	}
	if (start == stop) {
		std::cerr << "Fatal: not enough frames to process" << std::endl;
		std::exit(-1);
	}
	if (stop > max_count) {
		stop = max_count;
	}
}

H5::H5File file;

void siginthandler(int param)
{
	std::cerr << std::endl;
	if (file.getId() > 0) {
		file.flush(H5F_SCOPE_GLOBAL);
	}
	std::exit(-1);
}


const char* dots[] = {
	"⠀", "⠁", "⠂", "⠃", "⠄", "⠅", "⠆", "⠇", "⠈", "⠉", "⠊", "⠋", "⠌", "⠍", "⠎", "⠏",
	"⠐", "⠑", "⠒", "⠓", "⠔", "⠕", "⠖", "⠗", "⠘", "⠙", "⠚", "⠛", "⠜", "⠝", "⠞", "⠟",
	"⠠", "⠡", "⠢", "⠣", "⠤", "⠥", "⠦", "⠧", "⠨", "⠩", "⠪", "⠫", "⠬", "⠭", "⠮", "⠯",
	"⠰", "⠱", "⠲", "⠳", "⠴", "⠵", "⠶", "⠷", "⠸", "⠹", "⠺", "⠻", "⠼", "⠽", "⠾", "⠿",
	"⡀", "⡁", "⡂", "⡃", "⡄", "⡅", "⡆", "⡇", "⡈", "⡉", "⡊", "⡋", "⡌", "⡍", "⡎", "⡏",
	"⡐", "⡑", "⡒", "⡓", "⡔", "⡕", "⡖", "⡗", "⡘", "⡙", "⡚", "⡛", "⡜", "⡝", "⡞", "⡟",
	"⡠", "⡡", "⡢", "⡣", "⡤", "⡥", "⡦", "⡧", "⡨", "⡩", "⡪", "⡫", "⡬", "⡭", "⡮", "⡯",
	"⡰", "⡱", "⡲", "⡳", "⡴", "⡵", "⡶", "⡷", "⡸", "⡹", "⡺", "⡻", "⡼", "⡽", "⡾", "⡿",
	"⢀", "⢁", "⢂", "⢃", "⢄", "⢅", "⢆", "⢇", "⢈", "⢉", "⢊", "⢋", "⢌", "⢍", "⢎", "⢏",
	"⢐", "⢑", "⢒", "⢓", "⢔", "⢕", "⢖", "⢗", "⢘", "⢙", "⢚", "⢛", "⢜", "⢝", "⢞", "⢟",
	"⢠", "⢡", "⢢", "⢣", "⢤", "⢥", "⢦", "⢧", "⢨", "⢩", "⢪", "⢫", "⢬", "⢭", "⢮", "⢯",
	"⢰", "⢱", "⢲", "⢳", "⢴", "⢵", "⢶", "⢷", "⢸", "⢹", "⢺", "⢻", "⢼", "⢽", "⢾", "⢿",
	"⣀", "⣁", "⣂", "⣃", "⣄", "⣅", "⣆", "⣇", "⣈", "⣉", "⣊", "⣋", "⣌", "⣍", "⣎", "⣏",
	"⣐", "⣑", "⣒", "⣓", "⣔", "⣕", "⣖", "⣗", "⣘", "⣙", "⣚", "⣛", "⣜", "⣝", "⣞", "⣟",
	"⣠", "⣡", "⣢", "⣣", "⣤", "⣥", "⣦", "⣧", "⣨", "⣩", "⣪", "⣫", "⣬", "⣭", "⣮", "⣯",
	"⣰", "⣱", "⣲", "⣳", "⣴", "⣵", "⣶", "⣷", "⣸", "⣹", "⣺", "⣻", "⣼", "⣽", "⣾", "⣿"
};


int main(int argc, char* argv[]) {
	signal(SIGINT, siginthandler);

	// Read config
	po::variables_map config;
	try {
		config = parse(argc, argv);
	} catch (const std::exception& e) {
		std::cerr << "Fatal: " << e.what() << std::endl;
		return -1;
	}

	// Load video
	cv::VideoCapture cap(config["input"].as<std::string>());
	if (!cap.isOpened()) {
		std::cerr << "Fatal: the input file cannot be open" << std::endl;
		return -1;
	}
	const int w = cap.get(CV_CAP_PROP_FRAME_WIDTH);
	const int h = cap.get(CV_CAP_PROP_FRAME_HEIGHT);
	const int c = cap.get(CV_CAP_PROP_FRAME_COUNT);
	const double fps = cap.get(CV_CAP_PROP_FPS);

	if (config.count("info")) {
		std::cout << config["input"].as<std::string>() << ": ";
		std::cout << w << "x" << h << " @ " << fps << "fps - duration: ";
		std::cout << trunc(c / fps / 3600) << ":" << fmod(trunc(c / fps / 60), 60) << ":" << fmod(c / fps, 60);
		std::cout << " (" << c << " frames)" << std::endl;
		std::exit(0);
	}

	int start, stop, step, count;
	frameLogic(config, start, stop, step, count, c);

	// Compute background image
	std::string bgpath;
	cv::Mat im, gm, bg;
	if (config.count("background")) {
		bgpath = config["background"].as<std::string>();
		bg = cv::imread(bgpath, CV_LOAD_IMAGE_GRAYSCALE);
	} else {
		bgpath = config["input"].as<std::string>() + ".background.jpg";
	}
	if (bg.empty()) {
		bg = cv::Mat(h, w, CV_32FC1, cv::Scalar::all(0));
		std::cout << "Computing background image:" << std::endl;
		std::cout << "    0%" << std::flush;
		for (int i = 0; cap.read(im); ++i) {
			cv::cvtColor(im, gm, CV_RGB2GRAY); // convert to grayscale
			cv::accumulate(gm, bg);
			std::cout << '\r' << dots[i%256] << ' ' << std::setw(3) << (i + 1) * 100 / c << "%" << std::flush;
		}
		bg /= c;
		bg.convertTo(bg, CV_8U);
		cv::imwrite(bgpath, bg);
		std::cout << std::endl;
	}

	// Rewind
	cap.set(CV_CAP_PROP_POS_FRAMES, 0);

	// Skip frames instead of setting CV_CAP_PROP_POS_FRAMES to avoid issue with keyframes
	for (int k = 1; k < start; ++k) cap.grab();

	// Prepare output files
	const bool live = config["live"].as<bool>();
	const bool vid = config.count("movie");
	const bool data = config.count("data");
	if (live) cv::namedWindow("fishFlow live");
	cv::VideoWriter vw;
	if (vid) {
		std::string path = config["movie"].as<std::string>();
		if (path.empty()) {
			path = config["input"].as<std::string>() + ".flow.avi";
		}
		vw.open(path, CV_FOURCC('M','J','P','G'), fps, cv::Size(w, h));
		if (!vw.isOpened()) {
			std::cerr << "Fatal: the output video file cannot be open" << std::endl;
			return -1;
		}
	}
	const int gw = config["grid.width"].as<int>();
	const int gh = config["grid.height"].as<int>();
	Plot plot(gw, gh);
	H5::DataSet velocity_dset;
	H5::DataSet density_dset;
	H5::CompType xy_dtype(2 * sizeof(float));
	H5::DataSpace mem_space, file_dspace;
	if (data) {
		std::string path = config["data"].as<std::string>();
		if (path.empty()) {
			path = config["input"].as<std::string>() + ".flow.h5";
		}
		file = H5::H5File(path, H5F_ACC_TRUNC);
		const hsize_t dims[3] = {
			static_cast<hsize_t>(count),
			static_cast<hsize_t>(gw),
			static_cast<hsize_t>(gh)
		};
		file_dspace = H5::DataSpace(3, &dims[0]);
		mem_space = H5::DataSpace(2, &dims[1]);
		xy_dtype.insertMember("x", 0 * sizeof(float), H5::PredType::NATIVE_FLOAT);
		xy_dtype.insertMember("y", 1 * sizeof(float), H5::PredType::NATIVE_FLOAT);
		velocity_dset = file.createDataSet("velocity", xy_dtype, file_dspace);
		density_dset = file.createDataSet("density", H5::PredType::NATIVE_UCHAR, file_dspace);
	}

	// Compute density and optical flow
	std::cerr << "Computing density and optical flow:" << std::endl;
	const cv::Size size(gw, gh);
	cv::Mat prev, next, mask, uv;
	cv::Mat sd(gh, gw, CV_8UC1), suv(gh, gw, CV_32FC2);
	// Gunnar Farneback’s Optical Flow options
	const double pyr_scale = 0.5;
	const int levels = 2;
	const int winSize = 45;
	const int iterations = 4;
	const int poly_n = 7;
	const double poly_sigma = 1.5;
	int flags = cv::OPTFLOW_FARNEBACK_GAUSSIAN;
	std::cout << "    0%" << std::flush;
	for (int i = start, j = 0; i <= stop && cap.read(im); i += step, ++j) {
		cv::cvtColor(im, gm, CV_RGB2GRAY); // convert to grayscale

		// Compute density
		gm = gm - bg + 255;
		prev = next;
		next = gm.clone();
		cv::threshold(gm, gm, 200, 255, cv::THRESH_BINARY);
		cv::GaussianBlur(gm, gm, cv::Size(95, 95), 0, 0);
		cv::addWeighted(gm, -4, gm, 0, 1024, gm);

		// Compute density mask
		cv::threshold(gm, mask, 40, 255, cv::THRESH_BINARY);

		// Compute optical flow
		if (i == start) continue; // requires two frames
		cv::calcOpticalFlowFarneback(prev, next, uv, pyr_scale, levels, winSize, iterations, poly_n, poly_sigma, flags);
		flags |= cv::OPTFLOW_USE_INITIAL_FLOW;

		if (live || vid) {
			cv::addWeighted(im, 0.5, color(gm), 0.5, 0, im);
			plot.plotVelocity(im, uv, mask);
		}

		if (live) {
			cv::imshow("fishFlow live", im);
			cv::waitKey(10);
		}

		if (vid) {
			vw.write(im);
		}

		if (data) {
			const hsize_t start[3] = { static_cast<hsize_t>(j), 0, 0 };
			const hsize_t count[3] = {
				1,
				static_cast<hsize_t>(gw),
				static_cast<hsize_t>(gh)
			};
			file_dspace.selectHyperslab(H5S_SELECT_SET, count, start);
			cv::resize(uv, suv, size);
			cv::flip(suv, suv, 0);
			cv::transpose(suv, suv);
			velocity_dset.write(suv.ptr(), xy_dtype, mem_space, file_dspace);
			cv::resize(gm, sd, size);
			cv::flip(sd, sd, 0);
			cv::transpose(sd, sd);
			density_dset.write(sd.ptr(), H5::PredType::NATIVE_UCHAR, mem_space, file_dspace);
		}

		std::cout << '\r' << dots[j%256] << ' ' << std::setw(3) << (j + 1) * 100 / count << "%" << std::flush;

		// Skip frames instead of setting CV_CAP_PROP_POS_FRAMES to avoid issue with keyframes
		for (int k = 1; k < step; ++k) cap.grab();
	}
	std::cout << std::endl;
	if (live) cv::destroyAllWindows();

	return 0;
}
