<?php

namespace spec;

use FunctionMock;
use PhpSpec\ObjectBehavior;
use Prophecy\Argument;
use DKANMigrateMigrate;

class DKANMigrateBaseValidatorSpec extends ObjectBehavior
{
    function it_is_initializable()
    {
        include('dkan_migrate_base.module');
        $this->shouldHaveType('DKANMigrateBaseValidator');
    }

    function it_should_create_resource_list() {
      // Necessary for file function.
      define('FILE_EXISTS_REPLACE', 1);
      FunctionMock::createMockFunctionDefinition('t');
      FunctionMock::stub('t', 'here we go');

      FunctionMock::createMockFunctionDefinition('drupal_http_request');
      $response = new \stdClass();
      $response->data = file_get_contents('spec/package_list');
      FunctionMock::stub('drupal_http_request', $response, array('http://demo.getdkan.com/api/3/action/package_list'));

      $response = new \stdClass();
      $response->data = file_get_contents('spec/wisconsin-polling-places');
      FunctionMock::stub('drupal_http_request', $response, array('http://demo.getdkan.com/api/3/action/package_show?id=wisconsin-polling-places'));

      $response = new \stdClass();
      $response->data = file_get_contents('spec/wisconsin-polling-places');
      FunctionMock::stub('drupal_http_request', $response, array('http://demo.getdkan.com/api/3/action/package_show?id=afghanistan-election-districts'));

      FunctionMock::createMockFunctionDefinition('file_unmanaged_save_data');
      FunctionMock::stub('file_unmanaged_save_data', '');

      $endpoint = 'http://demo.getdkan.com/api/3/action/';
      $fileName = 'public://ckan-migrate-resource_list';
      $resourceIds = dkan_migrate_base_create_resource_list_items($endpoint, $fileName);
      $this->dkan_migrate_base_create_resource_list_items($endpoint, $fileName)->shouldHaveCount(2);
      $this->dkan_migrate_base_create_resource_list_items($endpoint, $fileName)->shouldHaveKey('result');
      $this->dkan_migrate_base_create_resource_list_items($endpoint, $fileName)->shouldHaveKey('help');
    }
}
