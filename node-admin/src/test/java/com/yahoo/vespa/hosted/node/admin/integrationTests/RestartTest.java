// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.hosted.node.admin.integrationTests;

import com.yahoo.metrics.simple.MetricReceiver;
import com.yahoo.vespa.hosted.dockerapi.ContainerName;
import com.yahoo.vespa.hosted.dockerapi.DockerImage;
import com.yahoo.vespa.hosted.dockerapi.metrics.MetricReceiverWrapper;
import com.yahoo.vespa.hosted.node.admin.ContainerNodeSpec;
import com.yahoo.vespa.hosted.node.admin.docker.DockerOperationsImpl;
import com.yahoo.vespa.hosted.node.admin.nodeadmin.NodeAdmin;
import com.yahoo.vespa.hosted.node.admin.nodeadmin.NodeAdminImpl;
import com.yahoo.vespa.hosted.node.admin.nodeadmin.NodeAdminStateUpdater;
import com.yahoo.vespa.hosted.node.admin.nodeagent.NodeAgent;
import com.yahoo.vespa.hosted.node.admin.nodeagent.NodeAgentImpl;
import com.yahoo.vespa.hosted.node.admin.util.Environment;
import com.yahoo.vespa.hosted.provision.Node;
import org.junit.Before;
import org.junit.Test;

import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.Collections;
import java.util.Optional;
import java.util.function.Function;

import static org.junit.Assert.assertTrue;
import static org.mockito.Matchers.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

/**
 * Tests that different wanted and current restart generation leads to execution of restart command
 *
 * @author musum
 */
public class RestartTest {

    @Test
    public void test() throws InterruptedException, UnknownHostException {
        CallOrderVerifier callOrder = new CallOrderVerifier();
        NodeRepoMock nodeRepositoryMock = new NodeRepoMock(callOrder);
        StorageMaintainerMock maintenanceSchedulerMock = new StorageMaintainerMock(callOrder);
        OrchestratorMock orchestratorMock = new OrchestratorMock(callOrder);
        DockerMock dockerMock = new DockerMock(callOrder);

        Environment environment = mock(Environment.class);
        when(environment.getConfigServerHosts()).thenReturn(Collections.emptySet());
        when(environment.getInetAddressForHost(any(String.class))).thenReturn(InetAddress.getByName("1.1.1.1"));

        Function<String, NodeAgent> nodeAgentFactory = (hostName) ->
                new NodeAgentImpl(hostName, nodeRepositoryMock, orchestratorMock, new DockerOperationsImpl(dockerMock, environment), maintenanceSchedulerMock);
        NodeAdmin nodeAdmin = new NodeAdminImpl(dockerMock, nodeAgentFactory, maintenanceSchedulerMock, 100,
                                                new MetricReceiverWrapper(MetricReceiver.nullImplementation));

        long wantedRestartGeneration = 1;
        long currentRestartGeneration = wantedRestartGeneration;
        nodeRepositoryMock.addContainerNodeSpec(createContainerNodeSpec(wantedRestartGeneration, currentRestartGeneration));

        NodeAdminStateUpdater updater = new NodeAdminStateUpdater(nodeRepositoryMock, nodeAdmin, 1, 1, orchestratorMock, "basehostname");

        // Wait for node admin to be notified with node repo state and the docker container has been started
        while (nodeAdmin.getListOfHosts().size() == 0) {
            Thread.sleep(10);
        }

        // Check that the container is started and NodeRepo has received the PATCH update
        assertTrue(callOrder.verifyInOrder(1000,
                                           "createContainerCommand with DockerImage: DockerImage { imageId=dockerImage }, HostName: host1, ContainerName: ContainerName { name=container }",
                                           "updateNodeAttributes with HostName: host1, NodeAttributes: NodeAttributes{restartGeneration=1, dockerImage=DockerImage { imageId=dockerImage }, vespaVersion='null'}"));

        wantedRestartGeneration = 2;
        currentRestartGeneration = 1;
        nodeRepositoryMock.updateContainerNodeSpec(createContainerNodeSpec(wantedRestartGeneration, currentRestartGeneration));

        assertTrue(callOrder.verifyInOrder(1000,
                                           "Suspend for host1",
                                           "executeInContainer with ContainerName: ContainerName { name=container }, args: [/opt/yahoo/vespa/bin/vespa-nodectl, restart]"));
        updater.deconstruct();
    }

    private ContainerNodeSpec createContainerNodeSpec(long wantedRestartGeneration, long currentRestartGeneration) {
        return new ContainerNodeSpec("host1",
                                     Optional.of(new DockerImage("dockerImage")),
                                     new ContainerName("container"),
                                     Node.State.active,
                                     "tenant",
                                     "docker",
                                     Optional.empty(),
                                     Optional.empty(),
                                     Optional.empty(),
                                     Optional.of(wantedRestartGeneration),
                                     Optional.of(currentRestartGeneration),
                                     Optional.of(1d),
                                     Optional.of(1d),
                                     Optional.of(1d));
    }
}