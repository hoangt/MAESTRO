
env = Environment()

includes = '''
              .
              lib/include
							lib/include/base
							lib/include/tools
							lib/include/DFSL
							lib/include/DSE
							lib/include/DFA
							lib/include/cost-analysis
							lib/include/AHW-model
							./lib/src
'''
env.Append(LINKFLAGS=['-lboost_program_options'])
env.Append(CXXFLAGS=['-std=c++17', '-lboost_program_options' ])
env.Append(LIBS=['-lboost_program_options'])

env.Append(CPPPATH = Split(includes))
#env.Program("maestro-top.cpp")
env.Program('maestro', ['maestro-top.cpp', 'lib/src/maestro.cpp' ])

