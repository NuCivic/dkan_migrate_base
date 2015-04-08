<?php

namespace spec;

use FunctionMock;
use PhpSpec\ObjectBehavior;
use Prophecy\Argument;

class DKANMigrateBaseValidatorSpec extends ObjectBehavior
{
    function it_is_initializable()
    {
        include('dkan_migrate_base.module');
        $this->shouldHaveType('DKANMigrateBaseValidator');
    }

    function it_should_create_resource_list() {
      $endpoint = 'http://demo.getdkan.com/api/3/action/';
      $fileName = 'public://ckan-migrate-resource_list';
      FunctionMock::createMockFunctionDefinition('t');
      FunctionMock::stub('t', 'abc');

      FunctionMock::createMockFunctionDefinition('drupal_http_request');
      $response = new \stdClass();
      $response->data = '{"help":"Return a list of the names of the site\u0027s datasets (packages).\n\n :param limit: if given, the list of datasets will be broken into pages of\n at most ``limit`` datasets per page and only one page will be returned\n at a time (optional)\n :type limit: int\n :param offset: when ``limit`` is given, the offset to start returning packages from\n :type offset: int\n\n :rtype: list of strings\n\n","success":true,"result":["wisconsin-polling-places"]}';
      //$response->data = '{"help":"Return a list of the names of the site\u0027s datasets (packages).\n\n :param limit: if given, the list of datasets will be broken into pages of\n at most ``limit`` datasets per page and only one page will be returned\n at a time (optional)\n :type limit: int\n :param offset: when ``limit`` is given, the offset to start returning packages from\n :type offset: int\n\n :rtype: list of strings\n\n","success":true,"result":["wisconsin-polling-places","us-national-foreclosure-statistics-january-2012","gold-prices-london-1950-2008-monthly","afghanistan-election-districts"]}';
      FunctionMock::stub('drupal_http_request', $response, array('http://demo.getdkan.com/api/3/action/package_list'));
      FunctionMock::createMockFunctionDefinition('drupal_http_request');

      $response = new \stdClass();
      $response->data = '{"help":"Return the metadata of a dataset (package) and its resources. :param id: the id or name of the dataset :type id: string","success":true,"result":{"id":"f8bbbb91-8529-4bd0-81f3-d7246778ef4d","name":"wisconsin-polling-places","title":"Wisconsin Polling Places","author_email":"admin@example.com","maintainer":"DKAN Demo","maintainer_email":"admin@example.com","license_title":"cc-by","notes":"\u003Cp\u003EPolling places in the state of Wisconsin\u003C\/p\u003E","url":"http:\/\/demo.getdkan.com\/dataset\/wisconsin-polling-places","state":"Active","private":"Published","revision_timestamp":"Wed, 04\/08\/2015 - 04:20","metadata_created":"Mon, 02\/11\/2013 - 05:13","metadata_modified":"Wed, 04\/08\/2015 - 04:20","creator_user_id":"367c8433-26c3-415a-8b2a-042f61955031","type":"Dataset","resources":[{"id":"8e74c37d-ca42-4509-9493-104883bcca07","revision_id":"","url":"http:\/\/demo.getdkan.com\/sites\/default\/files\/Polling_Places_Madison_0.csv","description":"\u003Cp\u003EThis is a list and map of polling places in Madison, WI.\u003C\/p\u003E\n\n\u003Cp\u003EOriginal data here: \n  \u003Ca href=\u0022https:\/\/data.cityofmadison.com\/Polling-Places\/Polling-Places\/rtyh-6ucr\u0022\u003Ehttps:\/\/data.cityofmadison.com\/Polling-Places\/Polling-Places\/rtyh-6ucr\u003C\/a\u003E\u003C\/p\u003E","format":"csv","state":"Active","revision_timestamp":"Wed, 04\/08\/2015 - 04:20","name":"Madison Polling Places","mimetype":"text\/csv","size":"17.06 KB","created":"Mon, 02\/11\/2013 - 05:16","resource_group_id":"","last_modified":"Date changed\tWed, 04\/08\/2015 - 04:20"}],"tags":[{"id":"1813f201-6a68-4e1e-aea3-80300cac1931","vocabulary_id":"2","name":"election"}],"groups":[{"description":"\u003Cp\u003EDKAN can plot both latitude and longitude as well as GeoJSON on a map.\u003C\/p\u003E\n","id":"535d1bd2-2691-46ad-9d62-613f5af9209b","image_display_url":"http:\/\/demo.getdkan.com\/sites\/default\/files\/6944276022_06ea83e528_0.jpg","title":"Geospatial Data Explorer Examples","name":"group\/geospatial-data-explorer-examples"}]}';

      FunctionMock::stub('drupal_http_request', $response, array('http://demo.getdkan.com/api/3/action/package_show?id=wisconsin-polling-places'));
      //FunctionMock::stub('drupal_http_request', $response);

      $resourceIds = dkan_migrate_base_create_resource_list_items($endpoint, $fileName);
      $list = $this->getFile($fileName);
      $this->resourceListCount($list)->shouldEqual(4);
      $this->resourceList($list)->shouldEqual($resourceIds['result']);
    }
}
