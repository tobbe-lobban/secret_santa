#include <sys/stat.h>
#include <sys/types.h>

#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <unordered_map>
#include <vector>

#include "unistd.h"

using namespace std;

using Permutation = vector<unsigned>;
using People = vector<string>;
using Constraints = unordered_map<string, People>;

//  i   perm[i]
//  0       2
//  1       1
//  2       0

unsigned factorial(unsigned n) {
  unsigned res{1};
  for (unsigned i{1}; i <= n; ++i) {
    res *= i;
  }
  return res;
}

void print_usage() {
  std::cout << "Usage:" << endl;
  std::cout << "./SecretSanta --help: display this message." << endl;
  std::cout << "./SecretSanta: writes secret Santas to current directory."
            << endl;
  std::cout
      << "./SecretSanta <fully-qualified path to directory>: writes secret "
         "Santas to given directory "
         "if it doesn't exist."
      << endl;
}

bool i_can_give_to_j(unsigned i, unsigned j, const People &peeps,
                     const Constraints &consts) {
  if (i >= peeps.size() || j >= peeps.size()) return false;
  if (i == j) return false;  // cant give to yourself
  if (auto it = consts.find(peeps[i]); it != consts.end()) {
    const auto &consts_i = it->second;
    if (ranges::contains(consts_i, peeps[j])) return false;
  }
  return true;
}

bool contains_all(const Permutation &perm, const People &peeps) {
  const auto nr_peeps = peeps.size();
  if (perm.size() != nr_peeps) {
    cerr << "contains_all() found that perm.sze() != peeps.size()" << endl;
    return false;
  }
  for (People::size_type i{0}; i < nr_peeps; ++i) {
    if (!ranges::contains(perm, i)) {
      cerr << "contains_all() found that perm was missing person: " << peeps[i]
           << endl;
      return false;
    }
  }
  return true;
}

bool verify_permutation(const Permutation &perm, const People &peeps,
                        const Constraints &consts) {
  if (!contains_all(perm, peeps)) return false;
  for (unsigned i = 0; i < peeps.size(); ++i) {
    if (!i_can_give_to_j(i, perm[i], peeps, consts)) {
      return false;
    }
    if (perm[perm[i]] == i) {  // no 2-cycles
      return false;
    }
  }
  return true;
}

optional<string> get_current_directory() {
  constexpr int max_path_length{1024};
  char cwd_buffer[max_path_length];
  if ((getcwd(cwd_buffer, sizeof(cwd_buffer)) != NULL) && (errno != ERANGE)) {
    const string cwd_string{cwd_buffer};
    return cwd_string;
  }
  return nullopt;
}

bool verify_constraints(const Constraints &consts, const People &peeps) {
  for (auto const &person : peeps) {
    auto const &person_constraints{consts.find(person)};
    if (person_constraints == consts.end()) {
      std::cout << "Error! Failed to find " << person << " in constraints."
                << std::endl;
      return false;
    }
    for (auto const &other : person_constraints->second) {
      if (!ranges::contains(peeps, other)) {
        std::cout << "Error! Failed to find " << other << " in constraints for "
                  << person << std::endl;
        return false;
      }
    }
  }
  return true;
}

template <typename T>
void print_v(const vector<T> &v) {
  std::for_each(v.cbegin(), v.cend(),
                [](const T &c) { std::cout << c << " "; });
  std::cout << std::endl;
}

optional<Permutation> generate_perm(const unsigned n, const People &peeps,
                                    const Constraints &consts,
                                    chrono::milliseconds timeout = 100ms) {
  if (n < 2) return nullopt;

  Permutation perm;
  perm.reserve(n);

  const auto time_start = chrono::system_clock::now();
  unsigned i = 0;
  while (i < n) {
    const auto current_time = chrono::system_clock::now();
    if ((current_time - time_start) > timeout) return nullopt;

    const unsigned j = rand() % n;
    if (!ranges::contains(perm, j) && i_can_give_to_j(i, j, peeps, consts)) {
      perm.push_back(j);
      ++i;
    }
  }
  return perm;
}

int main(int argc, char const *argv[]) {
  if (argc < 2) {
    print_usage();
    return EXIT_FAILURE;
  }

  // add people ... 
  const People people = {"p1", "p2", "p3"};

  // individual constraints, add them below ...
  Constraints constraints{};
  // clang-format off
  constraints.insert({"p1", {"p2"}});
  constraints.insert({"p2", {"p3"}});
  constraints.insert({"p3", {}});
  // clang-format on

  if (!verify_constraints(constraints, people)) {
    std::cout << "Verifying constraints failed, exiting!" << std::endl;
    return EXIT_FAILURE;
  }

  const auto cwd_string{get_current_directory()};
  if (!cwd_string.has_value()) {
    std::cout << "Failed to get current working directory. Exiting!" << endl;
    return EXIT_FAILURE;
  }

  auto output_dir_string{cwd_string.value()};
  if (argc == 2) {
    try {
      const string user_supplied_output_dir_string{argv[1]};
      output_dir_string = user_supplied_output_dir_string;

      struct stat info;
      if (!stat(output_dir_string.data(), &info)) {
        std::cout << "Error: Directory " << output_dir_string
                  << " already exists, exiting!" << endl;
        return EXIT_FAILURE;
      }
    } catch (const exception &e) {
      std::cout << "Exception getting output directory: " << e.what() << endl;
      return EXIT_FAILURE;
    }
  }
  filesystem::path fs_output_path{output_dir_string};
  if (filesystem::create_directories(fs_output_path)) {
    std::cout << "Created directory: " << fs_output_path.c_str() << endl;
  } else {
    std::cout << "Failed to create directy: " << fs_output_path.c_str() << endl;
    return EXIT_FAILURE;
  }

  //   seeds the pseudo - random number generator used by std::rand() with the
  //   value seed.

  srand(time(0));

  const auto nr_people = people.size();
  int fail_count = 0;
  const chrono::duration timeout{60s};

  optional<Permutation> found_perm{nullopt};

  const auto time_start = chrono::system_clock::now();
  auto current_time = chrono::system_clock::now();

  while (!found_perm) {
    found_perm = generate_perm(nr_people, people, constraints);
    if (!found_perm) {
      ++fail_count;
    } else if (!verify_permutation(*found_perm, people, constraints)) {
      found_perm = nullopt;
      ++fail_count;
    }
    current_time = chrono::system_clock::now();
    if (timeout < (current_time - time_start)) break;
  }

  if (!found_perm) {
    cerr << "TIMEOUT! Failed to find permutation." << endl;
    return EXIT_FAILURE;
  }

  const auto elapsed = current_time - time_start;

  for (unsigned i{0}; i < nr_people; ++i) {
    const auto file_name = output_dir_string + "/" + people[i];
    ofstream secret_santa_fstream(file_name);
    if (secret_santa_fstream.is_open()) {
      secret_santa_fstream << people[found_perm->at(i)] << endl;
      secret_santa_fstream.close();
    } else {
      cerr << "FAILED TO OPEN FILE: " << file_name << endl;
      return EXIT_FAILURE;
    }
  }

  const string file_name = output_dir_string + "/details.txt";
  ofstream secret_santa_fstream(file_name);
  if (secret_santa_fstream.is_open()) {
    for (unsigned i{0}; i < nr_people; ++i) {
      secret_santa_fstream << people[i] << " -> " << people[found_perm->at(i)]
                           << endl;
    }
    secret_santa_fstream << "\nNumber of people: " << people.size() << endl;
    secret_santa_fstream << "Total permutations: " << factorial(people.size())
                         << endl;

    secret_santa_fstream << "\nGlobal search time: "
                         <<  chrono::duration_cast<chrono::duration<double, ratio<1,1'000>>>(elapsed).count() << " ms" << endl;
    secret_santa_fstream << "Number of failed searches: " << fail_count << endl;

    std::cout << "DONE!" << endl;
  } else {
    std::cout << "FAILED TO OPEN FILE: " << file_name << endl;
  }

  return EXIT_SUCCESS;
}
