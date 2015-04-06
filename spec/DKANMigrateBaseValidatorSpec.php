<?php

namespace spec;

use PhpSpec\ObjectBehavior;
use Prophecy\Argument;

class DKANMigrateBaseValidatorSpec extends ObjectBehavior
{
    function it_is_initializable()
    {
        $this->shouldHaveType('DKANMigrateBaseValidator');
    }

    function it_should_create_resource_list() {
      $endpoint = 'http://demo.getdkan.com/api/3/action/';
      $fileName = 'public://ckan-migrate-resource_list';
      $resourceIds = dkan_migrate_base_create_resource_list_items($endpoint, $fileName);
      $list = $this->getFile($fileName);
      $this->resourceListCount($list)->shouldEqual(4);
      $this->resourceList($list)->shouldEqual($resourceIds['result']);
    }
}
