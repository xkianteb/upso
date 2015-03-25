# upso
Urban Planning Simulation and Optimization (upso)

We hypothesize that there are specific behavioral patterns that agents can use that measurably optimize efficient flow through a parameterized boundary configuration to a goal location. Further, we propose that these measurements can be computed by implementing a scalable, parallelized, framework with flexible work assignment and efficient communication.
	
	Our research is to determine optimal behavior patterns for agents moving through a multi-floor building. This work will first use the amount of time needed for a single agent to reach his predetermined goal location inside a parameterized floor configuration. Then, we will increase the number of agents present in the simulation until a significant increase of completion time needed occurs. We will then attach behavioral heuristics to each agent and measure our hypothesized decrease in completion time. Additionally, we plan to measure the effect that certain non-conformists have on the overall completion time. Further, we plan to implement various random events and simulate the effect these have on the agents.
    - Emergency: globally assign goal state for agents to the floor exit location
    - Delay: Temporarily impede a specific location’s traffic flow (Janitor waxing the floors…)
    - Rule breaking:  there are always bad actors in society, some who don’t obey the laws for their own selfish gain.  How does this affect the rest of the goal seekers, and how does it affect the rule breaker themselves?  To simulate this we create some of our agents with their normal “rules” turned off and see what behavior emerges.  

	Additionally, through varying the floor configuration, we can converge to an optimal hallway width for specific agent densities.

	The framework will be implemented using MPI. This framework will divide the floor area into batches of work for each available core in order to more efficiently compute path-finding in parallel. This problem is non-trivial and is not embarrassingly parallel, as the inter-node communication is complex and will be substantial over the course of a simulation.

	Our novel contribution is in our scalable implementation of the pathfinding and computation framework. Our research on related works revealed a lack of flexible and efficient multi-agent path-finding simulations using parallelized MPI. 
